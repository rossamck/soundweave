#include "RTP.h"

RTP::RTP(const char* destination_ip, int destination_port, uint8_t payload_type) {
  // Set up the destination address
  memset(&dest_addr_, 0, sizeof(dest_addr_));
  dest_addr_.sin_family = AF_INET;
  dest_addr_.sin_port = htons(destination_port);
  if (inet_aton(destination_ip, &dest_addr_.sin_addr) == 0) {
    throw std::invalid_argument("Invalid destination IP address");
  }

  // Initialize the RTP header fields
  rtp_header_.version = 2;
  rtp_header_.padding = 0;
  rtp_header_.extension = 0;
  rtp_header_.csrc_count = 0;
  rtp_header_.marker = 0;
  rtp_header_.payload_type = payload_type;
  rtp_header_.sequence_number = 0;
  rtp_header_.timestamp = 0;
  rtp_header_.ssrc = 0;
}

RTP::~RTP() {
  // close(sockfd_);
}

void RTP::SendPacket(const void* data, int size, int sockfd_) {
  // Increase the sequence number
  rtp_header_.sequence_number++;

  // Update the timestamp
  using namespace std::chrono;
  auto now = time_point_cast<microseconds>(system_clock::now());
  auto value = now.time_since_epoch();
  uint32_t timestamp = (uint32_t)(value.count() & 0xFFFFFFFF);
  rtp_header_.timestamp = timestamp;
  std::cout << "Current time: " << timestamp << std::endl;

  // Set up the buffer to hold the RTP header and payload
  uint8_t buffer[sizeof(rtp_header_) + size];

  // Copy the RTP header into the buffer
  memcpy(buffer, &rtp_header_, sizeof(rtp_header_));

  // Copy the payload into the buffer after the RTP header
  memcpy(buffer + sizeof(rtp_header_), data, size);

  // Send the packet
  if (sendto(sockfd_, buffer, sizeof(buffer), 0, (struct sockaddr*)&dest_addr_, sizeof(dest_addr_)) < 0) {
    throw std::runtime_error("Error sending RTP packet");
  }
}

 int RTP::GetLocalPort() {
    // Get the local address and port associated with the socket
    struct sockaddr_in sock_addr;
    socklen_t sock_addr_len = sizeof(sock_addr);
    if (getsockname(sockfd_, (struct sockaddr*)&sock_addr, &sock_addr_len) < 0) {
      throw std::runtime_error("Error getting sock name");
    }

    // Convert the local port to host byte order and return it
    return ntohs(sock_addr.sin_port);
  }
