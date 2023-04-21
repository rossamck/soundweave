// client.cpp
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main() {

    std::cout << "Starting client!" << std::endl;
    try {
        boost::asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), "64.226.97.53", "13579").begin();

        udp::socket socket(io_context);
        socket.open(udp::v4());

        socket.send_to(boost::asio::buffer("connect"), server_endpoint);

        char data[1024];
        udp::endpoint sender_endpoint;
        size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);

        std::string received_data(data, len);
        std::string peer_ip = received_data.substr(0, received_data.find(':'));
        int peer_port = std::stoi(received_data.substr(received_data.find(':') + 1));

        udp::endpoint peer_endpoint(boost::asio::ip::address::from_string(peer_ip), peer_port);

               // Send a message to the peer client
        std::string message = "Hello, peer!";
        socket.send_to(boost::asio::buffer(message), peer_endpoint);

        // Receive a message from the peer client
        char incoming_data[1024];
        udp::endpoint incoming_endpoint;
        size_t received_len = socket.receive_from(boost::asio::buffer(incoming_data), incoming_endpoint);

        std::string received_message(incoming_data, received_len);
        std::cout << "Received from peer: " << received_message << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

