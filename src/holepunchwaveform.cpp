#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <SDL2/SDL.h>
#include <cstdlib> // Add this header


#include "Networking/holepunch.h"

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

int main(int argc, char* argv[]) { // Modify the main function to accept command-line arguments

    bool use_local_client = false;
    int receiverPort = 12345;
    int sockfd;

    if (argc > 1 && strcmp(argv[1], "--local") == 0) {
        use_local_client = true;
    }

    if (use_local_client) {
        // Use local client
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
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
        if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            perror("bind");
            return 1;
        }
    } else {
        // Use original code
        std::cout << "Initialising..." << std::endl;
        ipInformation otherClient;
        otherClient = connectToClient();
        std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
        std::cout << "Socket is: " << otherClient.sock << std::endl;

        sockfd = otherClient.sock;
    }



  // Initialize SDL2
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "Error initializing SDL2: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create the SDL2 window and renderer
  SDL_Window* window = SDL_CreateWindow("Audio Waveform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    std::cerr << "Error creating SDL2 window: " << SDL_GetError() << std::endl;
    return 1;
  }
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    std::cerr << "Error creating SDL2 renderer: " << SDL_GetError() << std::endl;
    return 1;
  }

  // // Create a UDP socket
  // int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  // if (sockfd < 0) {
  //   throw std::runtime_error("Error opening socket");
  // }

  // // Set up the local address
  // struct sockaddr_in local_addr;
  // memset(&local_addr, 0, sizeof(local_addr));
  // local_addr.sin_family = AF_INET;
  // local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // local_addr.sin_port = htons(receiverPort);

  //   // Bind the socket to the local address
  // if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
  //   throw std::runtime_error("Error binding socket to local address");
  // }

  // int sockfd = otherClient.sock;


  while (true) {
  // Set up the remote address
  struct sockaddr_in remote_addr;
  socklen_t remote_addr_len = sizeof(remote_addr);
  // Receive an RTP packet
  uint8_t buffer[kBufferSize];
  int size = recvfrom(sockfd, buffer, kBufferSize, 0, (struct sockaddr*)&remote_addr, &remote_addr_len);
  if (size < 0) {
    throw std::runtime_error("Error receiving RTP packet");
  }

  // Check for keyboard events
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      exit(0);
    } else if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_q) {
        exit(0);
      }
    }
  }

    // std::cout << "Buffer size = " << size << std::endl;

    // Separate the RTP header and payload
    RTPHeader* header = (RTPHeader*)buffer;
    uint8_t* payload = buffer + sizeof(RTPHeader);
    int payload_size = size - sizeof(RTPHeader);

    // Extract the audio samples as 16-bit signed integers from the payload buffer
    int16_t* samples = (int16_t*)payload;
    int num_samples = payload_size / sizeof(int16_t);

    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // Set the color for the waveform
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    // Calculate the vertical scale factor to fit the waveform in the window
    float scale_factor = (float)600 / ((float)32768/4);
    

    // Draw the waveform
    for (int i = 0; i < num_samples - 1; i++) {
      int x1 = (int)((float)i / (float)num_samples * 800.0f);
      int y1 = (int)((float)samples[i] * scale_factor + 300.0f);
      int x2 = (int)((float)(i + 1) / (float)num_samples * 800.0f);
      int y2 = (int)((float)samples[i + 1] * scale_factor + 300.0f);
      SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }

    // Update the window
    SDL_RenderPresent(renderer);
  }

  // Clean up SDL2
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
