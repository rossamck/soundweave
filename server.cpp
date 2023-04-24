#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <vector>
#include <thread>

using boost::asio::ip::tcp;
using namespace std::chrono_literals;



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

  ClientInfo(int id, const std::string& public_ip, const std::string& local_ip = "", unsigned short port = 0)
      : id(id), public_ip(public_ip), local_ip(local_ip), target_ip(public_ip), action(""), port(port), udp_port(port + 1) {}

 // Serialize the struct into a string
std::string serialize() const
{
  std::ostringstream oss;
  oss << id << " " << public_ip << " " << local_ip << " " << target_ip << " ";
  if (action.empty()) {
    oss << "_";
  } else {
    oss << action;
  }
  oss << " " << port << " " << udp_port;
  return oss.str();
}


  // Deserialize the string back into the struct
  static ClientInfo deserialize(const std::string& str)
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

class SessionManager
{
public:
  SessionManager() : next_session_id_(1) {}

  int create_session()
  {
    int session_id = next_session_id_++;
    sessions_[session_id] = std::vector<ClientInfo>();
    return session_id;
  }

  void add_client_to_session(int session_id, const ClientInfo &client_info)
  {
    sessions_[session_id].push_back(client_info);
  }

  std::vector<ClientInfo> get_clients_in_session(int session_id) const
  {
    return sessions_.at(session_id);
  }

  bool client_ip_exists(int session_id, const std::string &public_ip)
  {
    for (const auto &client : sessions_[session_id])
    {
      if (client.public_ip == public_ip)
      {
        return true;
      }
    }
    return false;
  }

  void print_clients_in_session(int session_id) const
  {
    const auto &clients = sessions_.at(session_id);
    std::cout << "Clients in session " << session_id << ":" << std::endl;
    for (const auto &client : clients)
    {
      client.print();
    }
  }

private:
  int next_session_id_;
  std::map<int, std::vector<ClientInfo>> sessions_;
};

class ClientSession : public std::enable_shared_from_this<ClientSession>
{
public:
  ClientSession(tcp::socket socket, SessionManager &session_manager, int session_id, const ClientInfo &client_info)
      : socket_(std::move(socket)), session_manager_(session_manager), session_id_(session_id), client_info_(client_info)
  {
  }

  void start()
  {
    // read_local_ip();
    start_check_in();

  }

 void send_self_info()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer("SELF_INFO:" + client_info_.serialize() + "\n"),
                             [this, self](const boost::system::error_code &ec, std::size_t)
                             {
                               if (!ec)
                               {
                                 read_message();
                               }
                             });
  }

void send_client_info_to_others(const std::string &message)
{
  auto self(shared_from_this());
  boost::asio::async_write(socket_, boost::asio::buffer("PEER_INFO:" + message),
                           [this, self](const boost::system::error_code &ec, std::size_t)
                           {
                             if (!ec)
                             {
                               read_message();
                             }
                           });
}


void read_local_ip(std::function<void()> callback)
{
  auto self(shared_from_this());
  boost::asio::async_read_until(socket_, buffer_, '\n',
                                [this, self, callback](const boost::system::error_code &ec, std::size_t)
                                {
                                  if (!ec)
                                  {
                                    std::istream is(&buffer_);
                                    std::string local_ip;
                                    std::getline(is, local_ip);

                                    // Update the client_info_ object with the received local IP
                                    client_info_.local_ip = local_ip;
                                    std::cout << "new test " << std::endl;
                                    client_info_.print();

                                    // Update the session_manager_ with the new client_info_
                                    session_manager_.add_client_to_session(session_id_, client_info_);

                                    send_self_info();  // Move this line here
                                    read_message();

                                    if (callback)
                                    {
                                      callback();
                                    }
                                  }
                                });
}


  ClientInfo client_info_;

private:
  void read_message()
  {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, '\n',
                                  [this, self](const boost::system::error_code &ec, std::size_t)
                                  {
                                    if (!ec)
                                    {
                                      std::istream is(&buffer_);
                                      std::string message;
                                      std::getline(is, message);

                                      if (message == "PING")
                                      {
                                        message = "PONG!\n";
                                      }
                                      else
                                      {
                                        message += "!\n";
                                      }

                                      write_message(message);
                                    }
                                  });
  }

  void write_message(const std::string &message)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(message),
                             [this, self](const boost::system::error_code &ec, std::size_t)
                             {
                               if (!ec)
                               {
                                 read_message();
                               }
                             });
  }

  void start_check_in()
  {
    auto self(shared_from_this());
    check_in_timer_.expires_after(5s);
    check_in_timer_.async_wait([this, self](const boost::system::error_code &ec)
                               {
                                   if (!ec) {
                                       // Print the clients in the session
                                       session_manager_.print_clients_in_session(session_id_);

                                       write_message("Server check-in\n");
                                       start_check_in();
                                   } });
  }





  

  tcp::socket socket_;
  boost::asio::streambuf buffer_;
  boost::asio::steady_timer check_in_timer_{socket_.get_executor()};

  SessionManager &session_manager_;
  int session_id_;
  
};

class Server
{
public:
  Server(boost::asio::io_context &io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    session_id_ = session_manager_.create_session();
    do_accept();
  }

private:
void do_accept()
{
  acceptor_.async_accept(
      [this](boost::system::error_code ec, tcp::socket socket)
      {
        if (!ec)
        {
          auto client_ip = socket.remote_endpoint().address().to_string();
          auto client_port = socket.remote_endpoint().port();
          ClientInfo client_info(next_client_id_++, client_ip, "", client_port);
          auto new_client = std::make_shared<ClientSession>(std::move(socket), session_manager_, session_id_, client_info);

          new_client->read_local_ip([this, new_client]()
          {
            // Send the information of the currently connected clients to the newly connecting client
            for (const auto &client : clients_)
            {
              new_client->send_client_info_to_others(client->client_info_.serialize() + "\n");
            }

            // Add the new client to the list of clients
            clients_.push_back(new_client);

            // Send the new client's information to all the other connected clients
            std::string new_client_info = new_client->client_info_.serialize() + "\n";
            for (const auto &client : clients_)
            {
              if (client != new_client)
              {
                client->send_client_info_to_others(new_client_info);
              }
            }
          });

          new_client->start();
        }

        do_accept();
      });
}


  tcp::acceptor acceptor_;
  int session_id_;
  int next_client_id_ = 1;

  SessionManager session_manager_;

  std::vector<std::shared_ptr<ClientSession>> clients_;
};

int main(int argc, char *argv[])
{
  try
  {
    std::cout << "Starting server" << std::endl;
    boost::asio::io_context io_context;
    short port = 12345;
    tcp::endpoint endpoint(tcp::v4(), port);

    Server server(io_context, port); // Update this line
    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  return 0;
}
