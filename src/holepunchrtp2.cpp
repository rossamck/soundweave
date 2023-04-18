#include <iostream>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <chrono>

#include "Networking/holepunch.h"
#include "Networking/RTP.h"
#include "AudioCapture/AudioCapture.h"

// Global RTP object
RTP *rtp = nullptr;
int socket_fd = -1;


void audio_data_callback(const std::vector<short>& audio_data) {
    // Print the number of samples in the audio data
    std::cout << "Captured audio data with " << audio_data.size() << " samples" << std::endl;

    // You can process the audio data further here, e.g., save it to a file, analyze it, etc.
       // Send audio data over RTP
    if (rtp != nullptr && socket_fd != -1) {
        rtp->SendPacket(audio_data.data(), audio_data.size() * sizeof(short), socket_fd);
    }
}


int main()
{
  std::cout << "Initialising..." << std::endl;
  ipInformation otherClient;
  otherClient = connectToClient();
  std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
  std::cout << "Socket is: " << otherClient.sock << std::endl;

  const char *destination_ip = otherClient.ip;
  int destination_port = otherClient.port;

  AudioCapture audioCapture("");
  audioCapture.register_callback(audio_data_callback);

  // Instantiate the RTP object
  rtp = new RTP(otherClient.ip, otherClient.port, 11); // Use the appropriate payload type
  socket_fd = otherClient.sock;

  while (true) {

  }
  return 0;
}