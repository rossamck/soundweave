#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <SDL2/SDL.h>
#include <cstdlib> // Add this header
#include <csignal>


#include "Networking/holepunch.h"
#include "AudioPlayback/AudioWaveform.h"

const int kBufferSize = 1024;

struct RTPHeader
{
  uint8_t version : 2;
  uint8_t padding : 1;
  uint8_t extension : 1;
  uint8_t csrc_count : 4;
  uint8_t marker : 1;
  uint8_t payload_type : 7;
  uint16_t sequence_number;
  uint32_t timestamp;
  uint32_t ssrc;
  uint32_t csrc[1];
};

void signalHandler(int signal) {
    std::cout << "Interrupt signal (" << signal << ") received.\n";
    // Terminate the program
    exit(signal);
}


int main(int argc, char *argv[])
{ // Modify the main function to accept command-line arguments
  signal(SIGINT, signalHandler);
  bool use_local_client = false;
  int receiverPort = 12345;
  int sockfd;

  if (argc > 1 && strcmp(argv[1], "--local") == 0)
  {
    use_local_client = true;
  }

  if (use_local_client)
  {
    // Use local client
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
      perror("socket");
      return 1;
    }

    // Set up the local address
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(receiverPort);

    // Bind the socket to the local address
    if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
      perror("bind");
      return 1;
    }
  }
  else
  {
    // Use original code
    std::cout << "Initialising..." << std::endl;
    ipInformation otherClient;
    otherClient = connectToClient();
    std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
    std::cout << "Socket is: " << otherClient.sock << std::endl;

    sockfd = otherClient.sock;
  }

  AudioWaveform waveform(800, 600);

    // while (waveform.processEvents()) {
      while (true) {
        waveform.processEvents();
        // Set up the remote address
        struct sockaddr_in remote_addr;
        socklen_t remote_addr_len = sizeof(remote_addr);
        // Receive an RTP packet
        uint8_t buffer[kBufferSize];
        int size = recvfrom(sockfd, buffer, kBufferSize, 0, (struct sockaddr *)&remote_addr, &remote_addr_len);
        if (size < 0) {
            throw std::runtime_error("Error receiving RTP packet");
        }

        // Separate the RTP header and payload
        RTPHeader *header = (RTPHeader *)buffer;
        uint8_t *payload = buffer + sizeof(RTPHeader);
        int payload_size = size - sizeof(RTPHeader);

        // Extract the audio samples as 16-bit signed integers from the payload buffer
        int16_t *samples = (int16_t *)payload;
        int num_samples = payload_size / sizeof(int16_t);

        // Draw the waveform
        waveform.drawWaveform(samples, num_samples);
    }

    return 0;
}