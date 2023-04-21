// client.cpp
#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include <vector>

using boost::asio::ip::udp;

class ClientManager
{
public:
    ClientManager(udp::socket &socket) : socket_(socket) {}

    void add_client(const udp::endpoint &client_endpoint)
    {
        clients_.push_back(client_endpoint);
    }

    void send_data_to_all(const std::string &message)
    {
        for (const auto &client_endpoint : clients_)
        {
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
    std::vector<udp::endpoint> clients_;
    std::vector<boost::thread> client_threads_;
};

int main()
{
    try
    {

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
                client_manager.add_client(peer_endpoint);
            }

            // Send a message to all connected clients
            std::string message = "Hello, peer! This is client " + client_id;
            client_manager.send_data_to_all(message);
            std::cout << "Sent message to peers: " << message << std::endl;

            // Receive messages from peer clients
            while (true)
            {
                char incoming_data[1024];
                udp::endpoint incoming_endpoint;
                size_t received_len = socket.receive_from(boost::asio::buffer(incoming_data), incoming_endpoint);
                std::cout << "Received a message from peer." << std::endl;

                std::string received_message(incoming_data, received_len);
                std::cout << "Message from peer: " << received_message << std::endl;
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
