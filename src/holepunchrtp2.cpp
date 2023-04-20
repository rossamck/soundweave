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
#include <cstdlib> // Add this header

#include "Networking/holepunch.h"
#include "Networking/RTP.h"
#include "AudioCapture/AudioCapture.h"

// Use a unique_ptr for the RTP object
std::unique_ptr<RTP> rtp;
int socket_fd = -1;

void audio_data_callback(const std::vector<short>& audio_data) {
    std::cout << "Captured audio data with " << audio_data.size() << " samples" << std::endl;

    if (rtp && socket_fd != -1) {
        rtp->SendPacket(audio_data.data(), audio_data.size() * sizeof(short), socket_fd);
    }
}

int main(int argc, char* argv[]) { // Modify the main function to accept command-line arguments

    bool use_local_client = false;
    if (argc > 1 && strcmp(argv[1], "--local") == 0) {
        use_local_client = true;
    }

    if (use_local_client) {
        // Use local client
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            perror("socket");
            return 1;
        }

        rtp = std::make_unique<RTP>("192.168.0.164", 12345, 11); // Replace with the local IP address and port
    } else {
        // Use original code
        std::cout << "Initialising..." << std::endl;
        ipInformation otherClient;
        otherClient = connectToClient();
        std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
        std::cout << "Socket is: " << otherClient.sock << std::endl;

        rtp = std::make_unique<RTP>(otherClient.ip, otherClient.port, 11);
        socket_fd = otherClient.sock;
    }

    AudioCapture audioCapture("", true, true); //set true for dummy audio
    audioCapture.register_callback(audio_data_callback);
    audioCapture.start();

    while (true) {
    }

    audioCapture.stop(); // Stop the audio capture before exiting

    return 0;
}
