#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

int main() {
    try {
        std::cout << "Initialising client" << std::endl;
        boost::asio::io_context io_context;

        udp::resolver resolver(io_context);
        udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), "64.226.97.53", "8080").begin();
        std::cout << "Rendezvous server endpoint resolved: " << server_endpoint << std::endl;

        udp::socket socket(io_context);
        socket.open(udp::v4());
        std::cout << "Client socket opened." << std::endl;

        socket.send_to(boost::asio::buffer("connect"), server_endpoint);
        std::cout << "Connection request sent to the rendezvous server." << std::endl;

        char data[1024];
        udp::endpoint sender_endpoint;
        size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);
        std::cout << "Received peer information from the rendezvous server." << std::endl;

        std::string received_data(data, len);
        std::string peer_ip = received_data.substr(0, received_data.find(':'));
        int peer_port = std::stoi(received_data.substr(received_data.find(':') + 1));

        udp::endpoint peer_endpoint(boost::asio::ip::address::from_string(peer_ip), peer_port);
        std::cout << "Peer endpoint: " << peer_endpoint << std::endl;

        // Send a message to the peer client
        std::string message = "Hello, peer!";
        socket.send_to(boost::asio::buffer(message), peer_endpoint);
        std::cout << "Sent message to peer: " << message << std::endl;

        // Receive a message from the peer client
        char incoming_data[1024];
        udp::endpoint incoming_endpoint;
        size_t received_len = socket.receive_from(boost::asio::buffer(incoming_data), incoming_endpoint);
        std::cout << "Received a message from peer." << std::endl;

        std::string received_message(incoming_data, received_len);
        std::cout << "Message from peer: " << received_message << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

