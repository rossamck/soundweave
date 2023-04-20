#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <chrono>
#include <memory>

#include "AudioCapture/AudioCapture.h"

// UDP socket and destination address
int udp_socket;
struct sockaddr_in dest_addr;

void audio_data_callback(const std::vector<short>& audio_data) {
    std::cout << "Captured audio data with " << audio_data.size() << " samples" << std::endl;

    // Send audio data over the UDP socket
    if (sendto(udp_socket, audio_data.data(), audio_data.size() * sizeof(short), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto");
    }
}

int main() {
    AudioCapture audioCapture("", true); //set true for default device
    audioCapture.register_callback(audio_data_callback);
    audioCapture.start();

    // Set up IP information
    const char *dest_ip = "192.168.0.164"; // Replace with the IP address of the machine running the Windows application
    int dest_port = 12345; // Replace with the desired port number

    // Create UDP socket
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("socket");
        return 1;
    }

    // Set up destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    while (true) {
    }

    audioCapture.stop(); // Stop the audio capture before exiting

    // Close the UDP socket
    close(udp_socket);

    return 0;
}
