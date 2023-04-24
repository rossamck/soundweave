#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::udp;

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: client <host> <port>" << std::endl;
      return 1;
    }

    boost::asio::io_context io_context;

    udp::resolver resolver(io_context);
    udp::endpoint endpoint = *resolver.resolve(udp::v4(), argv[1], argv[2]).begin();

    udp::socket socket(io_context, udp::v4());

    std::string message = "Hello from UDP client!\n";
    socket.send_to(boost::asio::buffer(message), endpoint);

    std::cout << "Message sent to " << endpoint << std::endl;

    return 0;
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
