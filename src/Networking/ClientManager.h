#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ClientInfo.h"
#include "RTPHeader.h"

using boost::asio::ip::udp;

class ClientManager {
 public:
  explicit ClientManager(udp::socket &socket) : socket_(socket) {}

  void add_client(const ClientInfo &client_info);

  void send_data_to_all(const std::string &message);

  void send_audio_to_all(const std::vector<short> &audio_data);

  void print_clients();
      void initialize(boost::asio::io_context& io_context, const std::string& server_ip, const std::string& server_port);


//test
    void parse_received_data(const std::vector<uint8_t>& data, RTPHeader& rtp_header, std::vector<short>& audio_data);

  

 private:
  void send_data_to_client(const udp::endpoint &client_endpoint);
  static std::vector<uint8_t> create_rtp_header(const RTPHeader& rtp_header);


  udp::socket &socket_;
  std::vector<ClientInfo> clients_;
  std::vector<boost::thread> client_threads_;
  std::mutex data_mutex_;
  std::condition_variable data_cv_;
  std::string message_;
  std::vector<short> audio_data_;
  bool stop_threads_ = false;
};

#endif  // CLIENT_MANAGER_H
