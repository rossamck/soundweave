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
    try
    {
        std::cout << "new client test" << std::endl;
        std::cout << "Initialising client" << std::endl;

        std::string local_ip = get_local_ip();
        std::cout << "Local ip = " << local_ip << std::endl;
        boost::asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), "64.226.97.53", "13579").begin();
        std::cout << "Rendezvous server endpoint resolved: " << server_endpoint << std::endl;

        udp::socket socket(io_context);
        socket.open(udp::v4());
        std::cout << "Client socket opened." << std::endl;

        std::string testmessage = "connect|" + local_ip;
        socket.send_to(boost::asio::buffer(testmessage.c_str(), testmessage.size()), server_endpoint);
        std::cout << "Connection request sent to the rendezvous server." << std::endl;

        char data[1024];
        udp::endpoint sender_endpoint;
        size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
        std::cout << "Received ID from the rendezvous server." << std::endl;

        std::string received_data(data, len);
        std::string id_prefix = "ID:";

        ClientManager client_manager(socket);

        if (received_data.substr(0, id_prefix.size()) == id_prefix)
        {
            std::string client_id = received_data.substr(id_prefix.size());
            std::cout << "Client ID: " << client_id << std::endl;

            len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
            std::cout << "Received peer information from the rendezvous server." << std::endl;

            std::string peer_data(data, len);
            std::vector<udp::endpoint> peer_endpoints;
            std::cout << "Received raw peer data: " << peer_data << std::endl;

            std::cout << "Client deserialising test!" << std::endl;
            ClientInfo receivedClientInfo = ClientInfo::deserialize(peer_data);
            receivedClientInfo.print();

            size_t pos = 0;
            while ((pos = peer_data.find(';')) != std::string::npos)
            {
                std::string peer_info = peer_data.substr(0, pos);
                std::string peer_ip = peer_info.substr(0, peer_info.find(':'));
                int peer_port = std::stoi(peer_info.substr(peer_info.find(':') + 1));
                peer_endpoints.emplace_back(boost::asio::ip::address::from_string(peer_ip), peer_port);
                peer_data.erase(0, pos + 1);
                std::cout << "Extracted peer IP: " << peer_ip << ", port: " << peer_port << std::endl;
            }

            // Add connected clients to the manager
            for (const auto &peer_endpoint : peer_endpoints)
            {
                // Create a ClientInfo instance and add it to the manager
                ClientInfo client_info(69, peer_endpoint.address().to_string(), "", peer_endpoint.port());
                client_info.print();
                client_manager.add_client(client_info);
            }

            // Send a message to all connected clients
            std::string message = "Hello, peer! This is client " + client_id;
            client_manager.send_data_to_all(message);
            std::cout << "Sent message to peers: " << message << std::endl;

            // Periodically send messages to all connected clients
            boost::thread periodic_sender(periodically_send_messages, std::ref(client_manager), client_id);

            // Receive messages from peer clients
            while (true)
            {
                char incoming_data[1024];
                udp::endpoint incoming_endpoint;
                size_t received_len = socket.receive_from(boost::asio::buffer(incoming_data), incoming_endpoint);
                std::cout << "Received a message from ";
                if (incoming_endpoint == server_endpoint)
                {
                    std::cout << "server: ";
                }
                else
                {
                    std::cout << "peer: ";
                }
                std::cout << incoming_data << std::endl;

                std::string received_message(incoming_data, received_len);
                if (received_message.substr(0, 4) == "NEW:")
                {
                    // Extract new client info and update the list of connected clients
                    std::string new_client_info = received_message.substr(4);
                    std::string new_client_ip = new_client_info.substr(0, new_client_info.find(':'));
                    int new_client_port = std::stoi(new_client_info.substr(new_client_info.find(':') + 1));
                    ClientInfo new_client(6969, new_client_ip, "", new_client_port);
                    client_manager.add_client(new_client);
                    std::cout << "New client joined the session: " << new_client_info << std::endl;
                }
                else if (received_message.substr(0, 7) == "Server:")
                {
                    // Handle messages from the server
                    std::cout << "Message from server: " << received_message.substr(7) << std::endl;
                }
                else
                {
                    // Handle normal messages from other clients
                    std::cout << "Message from peer: " << received_message << std::endl;
                }
                client_manager.print_clients();
            }
        }
        else
        {
            std::cerr << "Failed to receive client ID from the server" << std::endl;
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}