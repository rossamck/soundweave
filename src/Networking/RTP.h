#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <chrono>
#include <cstring>
#include <iostream>

class RTP {
public:
  // Constructor
  RTP(const char* destination_ip, int destination_port, uint8_t payload_type);

  // Destructor
  ~RTP();

  // Send an RTP packet
  void SendPacket(const void* data, int size, int sockfd_);

  // Get local port
  int GetLocalPort();

private:
  int sockfd_;
  struct sockaddr_in dest_addr_;

  struct {
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t extension : 1;
    uint8_t csrc_count : 4;
    uint8_t marker : 1;
    uint16_t payload_type : 11;
    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
  } rtp_header_;
};
