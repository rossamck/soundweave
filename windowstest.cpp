#include <SDL.h>
#include <iostream>
#include <vector>
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

int main(int argc, char* argv[]) {
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create UDP socket
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
                return 1;
    }

    // Bind socket to a port
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345); // Replace with the port number used in the audio capture program
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(udp_socket);
        WSACleanup();
        return 1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create SDL window
    SDL_Window *window = SDL_CreateWindow("Audio Waveform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main loop
    bool quit = false;
    SDL_Event event;
    std::vector<short> audio_data;

    while (!quit) {
        // Handle SDL events
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                quit = true;
            }
        }

        // Receive audio data
        char buffer[4096];
        sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);

        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received > 0) {
            audio_data = std::vector<short>((short *)buffer, (short *)(buffer + bytes_received));
        }

        // Render waveform
SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
SDL_RenderClear(renderer);

int prevX = 0; // Update the starting position to the left side of the screen
int prevY = SCREEN_HEIGHT / 2;

SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
for (size_t i = 0; i < audio_data.size(); ++i) {
    int x = i * SCREEN_WIDTH / audio_data.size();
    int y = SCREEN_HEIGHT / 2 - audio_data[i] * SCREEN_HEIGHT / (4 * 32767); // Increase the scaling factor from 2 to 4

    SDL_RenderDrawLine(renderer, prevX, prevY, x, y);

    prevX = x;
    prevY = y;
}

SDL_RenderPresent(renderer);

    }

        // Clean up
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        // Close the UDP socket
        closesocket(udp_socket);

        // Cleanup Winsock
        WSACleanup();

        return 0;
    }


