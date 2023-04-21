// server.cpp
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main() {
    std::cout << "Starting server!" << std::endl;
    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), 13579));

        std::vector<udp::endpoint> clients;

        while (true) {
            char data[1024];
            udp::endpoint sender_endpoint;
            size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);

            if (std::string(data, len) == "connect") {
                clients.push_back(sender_endpoint);
                std::cout << "Client connected: " << sender_endpoint << std::endl;

                if (clients.size() == 2) {
                    std::string client1_info = clients[1].address().to_string() + ":" + std::to_string(clients[1].port());
                    std::string client2_info = clients[0].address().to_string() + ":" + std::to_string(clients[0].port());
                    socket.send_to(boost::asio::buffer(client1_info), clients[0]);
                    socket.send_to(boost::asio::buffer(client2_info), clients[1]);
                    clients.clear();
                }
            }
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
