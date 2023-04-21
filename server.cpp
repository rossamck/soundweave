// server.cpp
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main() {
    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), 13579));
        std::cout << "Server initialized, waiting for clients." << std::endl;

        std::vector<udp::endpoint> clients;
        int client_id = 1;

        while (true) {
            char data[1024];
            udp::endpoint sender_endpoint;
            size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);

            std::string received_data(data, len);
            std::cout << "Received data from client: " << received_data << std::endl;

            if (received_data == "connect") {
                clients.push_back(sender_endpoint);
                std::cout << "Client connected: " << sender_endpoint << std::endl;

                // Send client ID to the connected client
                std::string id_message = "ID:" + std::to_string(client_id);
                socket.send_to(boost::asio::buffer(id_message), sender_endpoint);
                std::cout << "Sent client ID to the connected client: " << client_id << std::endl;
                client_id++;

                if (clients.size() == 2) {
                    std::string client1_info = clients[1].address().to_string() + ":" + std::to_string(clients[1].port());
                    std::string client2_info = clients[0].address().to_string() + ":" + std::to_string(clients[0].port());
                    socket.send_to(boost::asio::buffer(client1_info), clients[0]);
                    socket.send_to(boost::asio::buffer(client2_info), clients[1]);
                    std::cout << "Sent client information to both clients." << std::endl;
                    clients.clear();
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
