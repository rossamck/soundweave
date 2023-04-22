// client.cpp
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <string>



#ifdef _WIN32
#include <boost/asio/ip/host_name.hpp>
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif


using boost::asio::ip::udp;

// Create a struct to store client information
struct ClientInfo {
    int id;
    std::string public_ip;
    std::string local_ip;
    std::string target_ip;
    std::string action;
    unsigned short port;

    ClientInfo() : id(0), public_ip(""), local_ip(""), target_ip(""), action(""), port(0) {}
    ClientInfo(int id, const std::string& public_ip, const std::string& local_ip, unsigned short port)
        : id(id), public_ip(public_ip), local_ip(local_ip), target_ip(public_ip), action(""), port(port) {}

    // Serialize the struct into a string
    std::string serialize() const {
        std::ostringstream oss;
        oss << id << " " << public_ip << " " << local_ip << " " << target_ip << " " << action << " " << port;
        return oss.str();
    }

    // Deserialize the string back into the struct
    static ClientInfo deserialize(const std::string& str) {
        std::istringstream iss(str);
        ClientInfo clientInfo;
        iss >> clientInfo.id >> clientInfo.public_ip >> clientInfo.local_ip >> clientInfo.target_ip >> clientInfo.action >> clientInfo.port;
        return clientInfo;
    }

    void print() const {
        std::cout << "ID: " << id << std::endl;
        std::cout << "Public IP: " << public_ip << std::endl;
        std::cout << "Local IP: " << local_ip << std::endl;
        std::cout << "Target IP: " << target_ip << std::endl;
        std::cout << "Action: " << action << std::endl;
        std::cout << "Port: " << port << std::endl;
    }
};

class ClientManager
{
public:
    ClientManager(udp::socket &socket) : socket_(socket) {}

    void add_client(const ClientInfo &client_info)
    {
        clients_.push_back(client_info);
        std::cout << "Added client with IP: " << client_info.target_ip << ", port: " << client_info.port << std::endl;
    }

    void send_data_to_all(const std::string &message)
    {
        for (const auto &client_info : clients_)
        {
            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client_info.target_ip), client_info.port);
            boost::thread client_thread(&ClientManager::send_data_to_client, this, client_endpoint, message);
            client_threads_.push_back(boost::move(client_thread));
        }
        for (auto &thread : client_threads_)
        {
            thread.join();
        }
        client_threads_.clear();
    }

    void print_clients()
    {
        std::cout << "Connected clients:" << std::endl;
        for (const auto &client_info : clients_)
        {
            std::cout << "- " << client_info.target_ip << ":" << client_info.port << std::endl;
        }
    }

private:
    void send_data_to_client(const udp::endpoint &client_endpoint, const std::string &message)
    {
        socket_.send_to(boost::asio::buffer(message), client_endpoint);
        std::cout << "Sent message to client endpoint: " << client_endpoint << std::endl;
    }

    udp::socket &socket_;
    std::vector<ClientInfo> clients_;
    std::vector<boost::thread> client_threads_;
};

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
    try {
        boost::asio::io_service ios;
        boost::asio::ip::tcp::resolver resolver(ios);
        boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
        boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

        boost::asio::ip::tcp::endpoint endpoint;
        while (it != boost::asio::ip::tcp::resolver::iterator()) {
            endpoint = *it++;
            if (endpoint.address().is_v4() && !endpoint.address().is_loopback()) {
                local_ip = endpoint.address().to_string();
                break;
            }
        }
    } catch (const std::exception &e) {
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
    if (client_info.action == "SELF") {
        std::cout << "Received own configuration, storing..." << std::endl;
    }

    // Initialize the ClientManager with the socket
    ClientManager client_manager(socket);

    // Periodically send messages to other clients
    boost::thread sender_thread(periodically_send_messages, boost::ref(client_manager), std::to_string(client_info.id));

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

    sender_thread.join();
    return 0;
}