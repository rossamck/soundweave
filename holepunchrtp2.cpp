#include <iostream>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <chrono>



#include "holepunch.h"

class RTP {
public:
  // Constructor
  RTP(const char* destination_ip, int destination_port, uint8_t payload_type) {


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

  // Destructor
  ~RTP() {
    // close(sockfd_);
  }

  // Send an RTP packet
  void SendPacket(const void* data, int size, int sockfd_) {
    // Increase the sequence number
    rtp_header_.sequence_number++;

    // Update the timestamp
    // Get the current time in microseconds
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

    // std::cout << "Printing sockfd_ to debug: " << sockfd_ << std::endl;

    // Send the packet
    if (sendto(sockfd_, buffer, sizeof(buffer), 0, (struct sockaddr*)&dest_addr_, sizeof(dest_addr_)) < 0) {
      throw std::runtime_error("Error sending RTP packet");
    }
    // std::cout << "Sending packet to " << inet_ntoa(dest_addr_.sin_addr) << ":" << ntohs(dest_addr_.sin_port) << std::endl;
  }

  int GetLocalPort() {
    // Get the local address and port associated with the socket
    struct sockaddr_in sock_addr;
    socklen_t sock_addr_len = sizeof(sock_addr);
    if (getsockname(sockfd_, (struct sockaddr*)&sock_addr, &sock_addr_len) < 0) {
      throw std::runtime_error("Error getting sock name");
    }

    // Convert the local port to host byte order and return it
    return ntohs(sock_addr.sin_port);
  }


  

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






int main() {





  std::cout << "Initialising..." << std::endl;
  ipInformation otherClient;
  otherClient = connectToClient();
  std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
  std::cout << "Socket is: " << otherClient.sock << std::endl;









  const char* destination_ip = otherClient.ip;
  int destination_port = otherClient.port;

  // Parse command line arguments
  // const char* destination_ip = "192.168.0.4";
  // int destination_port = 12345;

  // Open the microphone
  snd_pcm_t* handle;
  if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0) < 0) {
    std::cerr << "Error opening microphone" << std::endl;
    return 1;
  }

  // Set microphone parameters
  snd_pcm_hw_params_t* params;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(handle, params);
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
 
  unsigned int sample_rate = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, nullptr);
  snd_pcm_hw_params_set_channels(handle, params, 1);
  snd_pcm_hw_params(handle, params);

  // Allocate memory for audio data
  const int buffer_size = 1024; //reduce this
  short* buffer = new short[buffer_size];

  // Create the RTP object
  RTP rtp(destination_ip, destination_port, 11); //check payload type!!!!


  std::cout << "Local port = " << otherClient.own_port << std::endl;

  // Capture audio and send it over RTP
  while (true) {
    snd_pcm_readi(handle, buffer, buffer_size);
    // std::cout << "Buffer size = " << buffer_size << std::endl;
    // std::cout << "Sent size = " << buffer_size * sizeof(short) << std::endl;
    // std::cout << "Buffer size usin sizeof(buffer) = " << sizeof(buffer) << std::endl;
    rtp.SendPacket(buffer, buffer_size * sizeof(short), otherClient.sock);
  }

  // Clean up
  delete[] buffer;
  snd_pcm_close(handle);

  return 0;
}
