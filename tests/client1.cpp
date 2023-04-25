// client.cpp
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind/bind.hpp>

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

void periodically_send_messages(ClientManager &client_manager, const std::string &client_id)
{
    while (true)
    {
        boost::this_thread::sleep(boost::posix_time::seconds(5));
        std::string message = "Periodic message from client " + client_id;
        client_manager.send_data_to_all(message);
    }
}

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

void tcp_connection_handler(boost::asio::io_context& io_context, const std::string& server_ip, unsigned short server_port)
{
    boost::asio::ip::tcp::socket tcp_socket(io_context);
    boost::asio::ip::tcp::endpoint server_tcp_endpoint(boost::asio::ip::address::from_string(server_ip), server_port);

    try
    {
        tcp_socket.connect(server_tcp_endpoint);
        std::cout << "Connecting to server on tcp socket" << std::endl;
        while (true)
        {
            // Communicate with the server using the TCP socket
            // ...
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}



int main()
{
    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));

    std::string local_ip = get_local_ip();
    std::cout << "Local IP: " << local_ip << std::endl;

    udp::resolver resolver(io_context);
    udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), "64.226.97.53", "13579").begin();

    // Send the "connect" message with the local IP
    std::string connect_message = "connect " + local_ip;
    socket.send_to(boost::asio::buffer(connect_message), server_endpoint);

    // Receive the client ID from the server
    char data[1024];
    
    udp::endpoint sender_endpoint;
    size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
    std::string received_data(data, len);
    std::cout << "Received initial data from server: " << received_data << std::endl;

    // Deserialize the received data into a ClientInfo object
    ClientInfo client_info = ClientInfo::deserialize(received_data);
    client_info.print();
    std::cout << "Action test = " << client_info.action << std::endl;
    if (client_info.action == "SELF")
    {
        std::cout << "Received own configuration, storing..." << std::endl;
        //connect to the new tcp socket
            unsigned short server_tcp_port = 24680;
                boost::thread tcp_thread(tcp_connection_handler, boost::ref(io_context), server_endpoint.address().to_string(), server_tcp_port);



    }

    // Initialize the ClientManager with the socket
    ClientManager client_manager(socket);

    // Periodically send messages to other clients
    boost::thread sender_thread(periodically_send_messages, boost::ref(client_manager), std::to_string(client_info.id));

    AudioCapture audioCapture("", true, false); // set true for dummy audio
    audioCapture.register_callback([&client_manager](const std::vector<short> &audio_data)
                                   { audio_data_callback(audio_data, client_manager); });
    audioCapture.start();

    // Main loop to receive data
    while (true)
    {
        size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
        std::string received_data(data, len);

        // Check if the message is from the server or a peer
        if (sender_endpoint == server_endpoint)
        {
            std::cout << "Received data from server: " << received_data << std::endl;

            // Deserialize the received data into a ClientInfo object
            ClientInfo received_client_info = ClientInfo::deserialize(received_data);

            // If the received client information is not for the current client, add it to the ClientManager
            if (received_client_info.id != client_info.id)
            {
                client_manager.add_client(received_client_info);
                client_manager.print_clients();
            }
        }
        else
        {
            // Handle peer messages
            std::cout << "Received data from peer (" << sender_endpoint << "): " << received_data << std::endl;

            
        }
    }

    // sender_thread.join();
    return 0;
}