#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

constexpr int MIN_NUM_CLIENTS = 2;

int main() {
    try {
        std::cout << "new server test" << std::endl;
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

                if (clients.size() >= MIN_NUM_CLIENTS) {
                    // Send the new client's info to all existing clients
                    std::string new_client_info = sender_endpoint.address().to_string() + ":" + std::to_string(sender_endpoint.port()) + ";";
                    for (const auto& client : clients) {
                        if (client != sender_endpoint) {
                            socket.send_to(boost::asio::buffer(new_client_info), client);
                        }
                    }

                    // Send all existing clients' info to the new client
                    std::string existing_clients_info;
                    for (const auto& client : clients) {
                        if (client != sender_endpoint) {
                            existing_clients_info += client.address().to_string() + ":" + std::to_string(client.port()) + ";";
                        }
                    }
                    socket.send_to(boost::asio::buffer(existing_clients_info), sender_endpoint);

                    // Send "NEW" prefix to all existing clients when a new client joins
                    if (clients.size() >= 3) {
                        std::string new_client_message = "NEW:" + new_client_info;
                        for (const auto& client : clients) {
                            if (client != sender_endpoint) {
                                socket.send_to(boost::asio::buffer(new_client_message), client);
                            }
                        }
                    }
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
