// client.cpp
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind/bind.hpp>
#include <functional>

#ifdef _WIN32
#include <boost/asio/ip/host_name.hpp>
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

#include "Networking/ClientManager.h"
#include "Networking/ClientInfo.h"
#include "Networking/RTP.h"
#include "AudioCapture/AudioCapture.h"

using boost::asio::ip::udp;
using namespace boost::placeholders;

void audio_data_callback(const std::vector<short> &audio_data, ClientManager &client_manager)
{
    // std::cout << "Captured audio data with " << audio_data.size() << " samples" << std::endl;
    // I WANT TO SEND THE DATA HERE
    client_manager.send_audio_to_all(audio_data);
}

class TcpClient
{
public:
    TcpClient(boost::asio::io_context &io_context, const std::string &server_ip, unsigned short server_tcp_port)
        : io_context_(io_context),
          socket_(io_context),
          resolver_(io_context),
          server_ip_(server_ip),
          server_tcp_port_(server_tcp_port)
    {
        std::cout << "Initialising TCP client" << std::endl;
    }

    void connect()
    {
        std::cout << "Connecting to server..." << std::endl;
        boost::asio::ip::tcp::resolver::query query(server_ip_, std::to_string(server_tcp_port_));
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);
        boost::asio::connect(socket_, endpoint_iterator);
        std::string testmessage = "SYN\n";
        write(testmessage);
    }

    void run()
    {
        std::cout << "Starting read!" << std::endl;
        read();
    }

    void read()
    {
        boost::asio::async_read_until(socket_, read_buffer_, '\n',
                                      boost::bind(&TcpClient::handle_read, this,
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code &error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::istream is(&read_buffer_);
            std::string received_data{std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>()};
            std::cout << "Received TCP data: " << received_data << std::endl;
            read(); // Continue reading data asynchronously
        }
        else
        {
            std::cerr << "Error reading TCP data: " << error.message() << std::endl;
        }
    }

    void write(const std::string &message)
    {
        boost::asio::async_write(socket_, boost::asio::buffer(message),
                                 [this](const boost::system::error_code &error, std::size_t bytes_transferred)
                                 {
                                     if (error)
                                     {
                                         std::cerr << "Error writing TCP data: " << error.message() << std::endl;
                                     }
                                 });
    }

private:
    boost::asio::io_context &io_context_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    std::string server_ip_;
    unsigned short server_tcp_port_;
    boost::asio::streambuf read_buffer_;
};

class Client
{
public:
    Client(boost::asio::io_context &io_context, const std::string &server_ip, unsigned short server_udp_port, unsigned short server_tcp_port)
        : io_context_(io_context),
          socket_(io_context, udp::endpoint(udp::v4(), 0)),
          server_ip_(server_ip),
          server_port_(server_udp_port),
          client_manager_(socket_),
          audio_capture_("", true, false),
          tcp_client_(io_context, server_ip, server_tcp_port) {}

    void run()
    {
        io_context_.run();
        start_async_receive();
    }

    void initialize()
    {
        // Get local IP address
        std::string local_ip = get_local_ip();
        std::cout << "Local IP: " << local_ip << std::endl;

        // Resolve server endpoint
        udp::resolver resolver(io_context_);
        server_endpoint = *resolver.resolve(udp::v4(), server_ip_, std::to_string(server_port_)).begin();

        // Send "connect" message with local IP
        std::string connect_message = "connect " + local_ip;
        socket_.send_to(boost::asio::buffer(connect_message), server_endpoint);

        // Receive client ID from the server
        char data[1024];
        size_t len = socket_.receive_from(boost::asio::buffer(data), sender_endpoint);
        std::string raw_received_data(data, len);
        std::cout << "Received initial data from server: " << raw_received_data << std::endl;

        std::cout << "SUBSTR TEST:" << raw_received_data.substr(0, 9) << std::endl;
        ;
        if (raw_received_data.substr(0, 9) == "OWN_INFO:")
        {
            std::cout << "RECEIVED OWN INFO!" << std::endl;
            std::string received_data = raw_received_data.substr(9);

            // Deserialize received data into a ClientInfo object
            ClientInfo client_info = ClientInfo::deserialize(received_data);
            client_info.print();
            boost::thread sender_thread(std::bind(&Client::periodically_send_messages, this, std::placeholders::_1), std::to_string(client_info.id));

            // Register the audio_data_callback with the AudioCapture object
            audio_capture_.register_callback(std::bind(&Client::audio_data_callback, this, std::placeholders::_1));
            audio_capture_.start();
        }

        // Start listening for incoming data
        start_async_receive();

        // Initialize the TCP client
        tcp_client_.connect();
        std::thread tcp_client_thread(&TcpClient::run, &tcp_client_);
        tcp_client_thread.detach();
    }

    void start_async_receive()
    {
        socket_.async_receive_from(boost::asio::buffer(data, max_length), sender_endpoint,
                                   boost::bind(&Client::handle_receive, this,
                                               boost::asio::placeholders::error,
                                               boost::asio::placeholders::bytes_transferred));
    }

    void handle_receive(const boost::system::error_code &error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::string received_data(data, bytes_transferred);

            // Check if the message is from the server or a peer
            if (sender_endpoint == server_endpoint)
            {
                // Deserialize the received data into a ClientInfo object

                ClientInfo received_client_info = ClientInfo::deserialize(received_data);

                // If the received client information is not for the current client, add it to the ClientManager
                client_manager_.add_client(received_client_info);
                client_manager_.print_clients();
            }
            else
            {
                // Handle peer messages
                std::cout << "Received data from peer (" << sender_endpoint << "): " << std::endl;
                // std::cout << "Received data from peer (" << sender_endpoint << "): " << received_data << std::endl;
            }

            start_async_receive(); // Start listening for new data again
        }
        else
        {
            std::cerr << "Error receiving UDP data: " << error.message() << std::endl;
        }
    }

private:
    void audio_data_callback(const std::vector<short> &audio_data)
    {
        // ...
        client_manager_.send_audio_to_all(audio_data);
    }

    // Other member functions
    std::string get_local_ip()
    {
        std::string local_ip;

#ifdef _WIN32
        try
        {
            boost::asio::io_service ios;
            boost::asio::ip::tcp::resolver resolver(ios);
            boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
            boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

            boost::asio::ip::tcp::endpoint endpoint;
            while (it != boost::asio::ip::tcp::resolver::iterator())
            {
                endpoint = *it++;
                if (endpoint.address().is_v4() && !endpoint.address().is_loopback())
                {
                    local_ip = endpoint.address().to_string();
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
#else
        struct ifaddrs *ifap, *ifa;
        struct sockaddr_in *sa;

        getifaddrs(&ifap);
        for (ifa = ifap; ifa; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
            {
                sa = (struct sockaddr_in *)ifa->ifa_addr;
                local_ip = inet_ntoa(sa->sin_addr);
                if (local_ip != "127.0.0.1")
                {
                    break;
                }
            }
        }
        freeifaddrs(ifap);
#endif

        return local_ip;
    }

    void periodically_send_messages(const std::string &client_id)
    {
        while (true)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(5));
            std::string message = "Periodic message from client " + client_id;
            client_manager_.send_data_to_all(message);
        }
    }

    boost::asio::io_context &io_context_;
    udp::socket socket_;
    std::string server_ip_;
    unsigned short server_port_;
    ClientManager client_manager_;
    AudioCapture audio_capture_;

    char data[1024];
    udp::endpoint sender_endpoint;
    udp::endpoint server_endpoint;

    TcpClient tcp_client_;
    const std::size_t max_length = 1024;
};

int main()
{
    boost::asio::io_context io_context;
    Client client(io_context, "64.226.97.53", 13579, 24680);
    std::cout << "Initialising client!" << std::endl;
    std::cout << std::endl;
    client.initialize();
    std::cout << "Starting main client loop" << std::endl;
    boost::thread main_loop_thread(&Client::run, &client);
    main_loop_thread.join();

    return 0;
}
