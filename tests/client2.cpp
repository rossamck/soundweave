#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <mutex>

#ifdef _WIN32
#include <boost/asio/ip/host_name.hpp>
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

struct ClientInfo
{
  int id;
  std::string public_ip;
  std::string local_ip;
  std::string target_ip;
  std::string action;
  unsigned short port;
  unsigned short udp_port;

  ClientInfo() : id(0), public_ip(""), local_ip(""), target_ip(""), action(""), port(0), udp_port(0) {}

  ClientInfo(int id, const std::string &public_ip, const std::string &local_ip = "", unsigned short port = 0)
      : id(id), public_ip(public_ip), local_ip(local_ip), target_ip(public_ip), action(""), port(port), udp_port(port + 1) {}

  // Serialize the struct into a string
  std::string serialize() const
  {
    std::ostringstream oss;
    oss << id << " " << public_ip << " " << local_ip << " " << target_ip << " ";
    if (action.empty())
    {
      oss << "_";
    }
    else
    {
      oss << action;
    }
    oss << " " << port << " " << udp_port;
    return oss.str();
  }

  // Deserialize the string back into the struct
  static ClientInfo deserialize(const std::string &str)
  {
    std::istringstream iss(str);
    ClientInfo clientInfo;
    iss >> clientInfo.id >> clientInfo.public_ip >> clientInfo.local_ip >> clientInfo.target_ip >> clientInfo.action >> clientInfo.port >> clientInfo.udp_port;
    return clientInfo;
  }

  void print() const
  {
    std::cout << "ID: " << id << std::endl;
    std::cout << "Public IP: " << public_ip << std::endl;
    std::cout << "Local IP: " << local_ip << std::endl;
    std::cout << "Target IP: " << target_ip << std::endl;
    std::cout << "Action: " << action << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "UDP Port: " << udp_port << std::endl;
  }
};

class PeerManager
{
public:
  PeerManager(udp::socket &socket) : socket_(socket) {}

  void add_client(const ClientInfo &client_info)
  {
    clients_.push_back(client_info);
    std::cout << "Added client with IP: " << client_info.target_ip << ", port: " << client_info.port << std::endl;
  }

  void send_data_to_all(const std::string &message)
  {
    for (const auto &client_info : clients_)
    {
      udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client_info.target_ip), client_info.udp_port);
      boost::thread client_thread(&PeerManager::send_data_to_client, this, client_endpoint, message);
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

  unsigned short get_bound_port() const
  {
    boost::system::error_code ec;
    udp::endpoint local_endpoint = socket_.local_endpoint(ec);
    if (ec)
    {
      std::cerr << "Error: " << ec.message() << std::endl;
      return 0;
    }
    return local_endpoint.port();
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

class Client
{

public:
  Client(const std::string &server_address, const std::string &server_port)
      : io_context_(),
        resolver_(io_context_),
        socket_(io_context_),
        udp_socket_(io_context_, udp::endpoint(udp::v4(), 0)), // Create an unbound UDP socket
        peer_manager_(udp_socket_)                             // Add a PeerManager instance
  {
    server_endpoint_ = *resolver_.resolve(server_address, server_port);
    udp_server_endpoint_ = *udp::resolver(io_context_).resolve(udp::v4(), server_address, "12346");
  }

  void connect()
  {
    boost::asio::connect(socket_, &server_endpoint_);

    // Send local IP address to the server
    boost::asio::write(socket_, boost::asio::buffer(get_local_ip() + "\n"));

    // Start the reader thread
    reader_thread_ = std::thread(&Client::read_messages, this);

    // send_initial_udp_message();
    udp_receiver_thread_ = std::thread(&Client::receive_udp_messages, this);
  }

  void send_message(const std::string &message)
  {
    boost::asio::write(socket_, boost::asio::buffer(message + "\n"));
  }

  void close()
  {
    socket_.shutdown(tcp::socket::shutdown_both);
    reader_thread_.join();
  }

  void send_message_to_peers(const std::string &message)
  {
    peer_manager_.send_data_to_all(message);
  }

  void send_initial_udp_message()
  {
    std::string initial_message = "Hello from UDP client!\n";
    udp_socket_.send_to(boost::asio::buffer(initial_message), udp_server_endpoint_);
    std::cout << "Initial message sent to UDP server at " << udp_server_endpoint_ << std::endl;

    // Print the local endpoint (including the randomly chosen port)
    boost::system::error_code ec;
    udp::endpoint local_endpoint = udp_socket_.local_endpoint(ec);
    if (ec)
    {
      std::cerr << "Error: " << ec.message() << std::endl;
    }
    else
    {
      std::cout << "Local UDP endpoint: " << local_endpoint << std::endl;
    }
  }

private:
  boost::asio::io_context io_context_;
  tcp::resolver resolver_;
  tcp::socket socket_;
  tcp::endpoint server_endpoint_;
  std::thread reader_thread_;

  std::unordered_map<int, ClientInfo> peer_clients_;
  std::mutex peer_clients_mutex_;
  ClientInfo self_info_;

  PeerManager peer_manager_; // Declare a PeerManager instance

  std::array<char, 1024> recv_buffer_;

  udp::socket udp_socket_; // Add this line
  udp::endpoint udp_server_endpoint_;

  std::thread udp_receiver_thread_;

  void receive_udp_messages()
  {
    while (true)
    {
      boost::system::error_code ec;
      udp::endpoint remote_endpoint;
      size_t bytes_received;
      udp_socket_.async_receive_from(
          boost::asio::buffer(recv_buffer_), remote_endpoint,
          [&](const boost::system::error_code &error, size_t bytes_transferred)
          {
            if (!error && bytes_transferred > 0)
            {
              bytes_received = bytes_transferred;
              std::string message(recv_buffer_.begin(), recv_buffer_.begin() + bytes_received);
              std::cout << "Received UDP message from " << remote_endpoint << ": " << message << std::endl;
            }
            else
            {
              std::cerr << "Error receiving UDP message: " << error.message() << std::endl;
            }
          });

      io_context_.run_one(); // Run the IO context to execute the receive operation
    }
  }

  std::string get_local_ip() const
  {
    std::string local_ip;

#ifdef _WIN32
    try
    {
      boost::asio::ip::tcp::resolver resolver(io_context_);
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
      boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

      boost::asio::ip::tcp::endpoint endpoint;
      while (it != boost::asio::ip::tcp::resolver::iterator())
      {
        endpoint = *it++;
        if (endpoint.address().is_v4() && !endpoint.address().is_loopback())
        {
          local_ip = endpoint.address().to_string();
          break;
        }
      }
    }
    catch (const std::exception &e)
    {
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

  void read_messages()
  {
    boost::asio::streambuf buffer;
    while (true)
    {
      boost::system::error_code ec;
      boost::asio::read_until(socket_, buffer, '\n', ec);
      if (ec)
        break;
      std::istream is(&buffer);
      std::string message;
      std::getline(is, message);

      if (message.find("PEER_INFO:") == 0)
      {
        // This is a peer information message
        std::string peer_info = message.substr(10); // Remove the "PEER_INFO:" prefix
        ClientInfo new_peer = ClientInfo::deserialize(peer_info);

        // Check if public IP matches self public IP and set target IP accordingly
        if (new_peer.public_ip == self_info_.public_ip)
        {
          std::cout << "WARNING: Connecting peer is on same network" << std::endl;

          new_peer.target_ip = new_peer.local_ip; // set target IP to local IP of the connecting peer
        }
        new_peer.print();

        // Store the new peer in the peer_clients map
        {
          std::unique_lock<std::mutex> lock(peer_clients_mutex_);
          peer_clients_[new_peer.id] = new_peer;
          peer_manager_.add_client(new_peer);
        }

        std::cout << "Received and stored peer info: " << peer_info << std::endl;
        std::cout << "sending initial message to peer..." << std::endl;

        std::string connect_message = "CONNECT";
        send_message_to_peers(connect_message);
        
        print_connected_clients();
      }
      else if (message.find("SELF_INFO:") == 0)
      {
        // This is self information message
        std::string self_info_str = message.substr(10);      // Remove the "SELF_INFO:" prefix
        self_info_ = ClientInfo::deserialize(self_info_str); // update self_info with the new self info received from the server

        // Bind the UDP socket to the received port
        // udp_socket_.open(udp::v4());
        // udp_socket_.bind(udp::endpoint(udp::v4(), self_info_.udp_port));

        std::cout << "Received self info: " << self_info_str << std::endl;
      }
      else
      {
        // This is a general message from the server
        std::cout << "Received: " << message << std::endl;
      }
    }
  }

  void print_connected_clients() const
  {
    std::cout << "Connected clients:" << std::endl;
    for (const auto &pair : peer_clients_)
    {
      pair.second.print();
    }
    std::cout << std::endl;
  }
};

int main(int argc, char *argv[])
{
  try
  {
    std::cout << "Starting client" << std::endl;
    Client client("64.226.97.53", "12345");
    client.send_initial_udp_message();
    client.connect();

    while (true)
    {
      std::string message;
      std::cout << "Enter a message: ";
      std::getline(std::cin, message);

      if (message.empty())
        break;

      client.send_message(message);
      std::string peer_message = "sup peer!";
      client.send_message_to_peers(peer_message);
    }

    client.close();
  }
  catch (std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}