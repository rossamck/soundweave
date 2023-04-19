#include <iostream>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <memory>

#include "Networking/holepunch.h"
#include "Networking/RTP.h"
#include "AudioCapture/AudioCaptureLinux.h" // Include the AudioCaptureLinux header file

// Use a unique_ptr for the RTP object
std::unique_ptr<RTP> rtp;
int socket_fd = -1;

void audio_data_callback(const std::vector<short>& audio_data) {
    std::cout << "Captured audio data with " << audio_data.size() << " samples" << std::endl;

    if (rtp && socket_fd != -1) {
        rtp->SendPacket(audio_data.data(), audio_data.size() * sizeof(short), socket_fd);
    }
}

int main() {
  std::cout << "Initialising..." << std::endl;
  ipInformation otherClient;
  otherClient = connectToClient();
  std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
  std::cout << "Socket is: " << otherClient.sock << std::endl;

  const char *destination_ip = otherClient.ip;
  int destination_port = otherClient.port;

  AudioCaptureLinux audioCapture("", false); // Use AudioCaptureLinux instead of AudioCapture
  audioCapture.register_callback(audio_data_callback);

  // Instantiate the RTP object with a unique_ptr
  rtp = std::make_unique<RTP>(otherClient.ip, otherClient.port, 11);
  socket_fd = otherClient.sock;

  while (true) {

  }
  return 0;
}
