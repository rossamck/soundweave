#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <vector>

using boost::asio::ip::udp;

// Define a ClientInfo struct
struct ClientInfo
{
    std::string ip;
    int port;

    ClientInfo(const std::string &ip, int port) : ip(ip), port(port) {}
};

class ClientManager
{
public:
    ClientManager(udp::socket &socket) : socket_(socket) {}

    void add_client(const ClientInfo &client_info)
    {
        clients_.push_back(client_info);
    }

    void send_data_to_all(const std::string &message)
    {
        for (const auto &client_info : clients_)
        {
            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client_info.ip), client_info.port);
            boost::thread client_thread(&ClientManager::send_data_to_client, this, client_endpoint, message);
            client_threads_.push_back(boost::move(client_thread));
        }
        for (auto &thread : client_threads_)
        {
            thread.join();
        }
        client_threads_.clear();
    }

private:
    void send_data_to_client(const udp::endpoint &client_endpoint, const std::string &message)
    {
        socket_.send_to(boost::asio::buffer(message), client_endpoint);
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
        std::string message = "Client " + client_id + " checking in";
        client_manager.send_data_to_all(message);
    }
}

int main()
{
    try
    {

        std::cout << "new client test" << std::endl;
        std::cout << "Initialising client" << std::endl;
        boost::asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), "64.226.97.53", "13579").begin();
        std::cout << "Rendezvous server endpoint resolved: " << server_endpoint << std::endl;

        udp::socket socket(io_context);
        socket.open(udp::v4());
        std::cout << "Client socket opened." << std::endl;

        std::string testmessage = "connect";
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

            size_t pos = 0;
            while ((pos = peer_data.find(';')) != std::string::npos)
            {
                std::string peer_info = peer_data.substr(0, pos);
                std::string peer_ip = peer_info.substr(0, peer_info.find(':'));
                int peer_port = std::stoi(peer_info.substr(peer_info.find(':') + 1));
                peer_endpoints.emplace_back(boost::asio::ip::address::from_string(peer_ip), peer_port);
                peer_data.erase(0, pos + 1);
            }

            // Add connected clients to the manager
            for (const auto &peer_endpoint : peer_endpoints)
            {
                // Create a ClientInfo instance and add it to the manager
                ClientInfo client_info(peer_endpoint.address().to_string(), peer_endpoint.port());
                client_manager.add_client(client_info);
            }

            // Start sending messages periodically to check connectivity
            boost::thread message_sender_thread(periodically_send_messages, std::ref(client_manager), client_id);

            // Receive messages from peer clients
            while (true)
            {
                char incoming_data[1024];
                udp::endpoint incoming_endpoint;
                size_t received_len = socket.receive_from(boost::asio::buffer(incoming_data), incoming_endpoint);
                std::cout << "Received a message from peer." << std::endl;

                std::string received_message(incoming_data, received_len);
                std::cout << "Message from peer: " << received_message << std::endl;

                std::string new_prefix = "NEW:";
                if (received_message.substr(0, new_prefix.size()) == new_prefix)
                {
                    std::string new_client_info = received_message.substr(new_prefix.size());
                    size_t pos = new_client_info.find(';');
                    std::string new_client_peer_info = new_client_info.substr(0, pos);
                    std::string new_client_ip = new_client_peer_info.substr(0, new_client_peer_info.find(':'));
                    int new_client_port = std::stoi(new_client_peer_info.substr(new_client_peer_info.find(':') + 1));

                    // Add the new client to the manager
                    ClientInfo new_client(new_client_ip, new_client_port);
                    client_manager.add_client(new_client);
                    std::cout << "Added new client to the manager: " << new_client_ip << ":" << new_client_port << std::endl;

                    // Send a message to the new client to establish a connection
                    std::string message = "Hello, new peer! This is client " + client_id;
                    client_manager.send_data_to_all(message);
                    std::cout << "Sent message to the new client: " << message << std::endl;
                }
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
