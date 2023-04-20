#include "AudioWaveform.h"
#include <stdexcept>
#include <iostream>

AudioWaveform::AudioWaveform(int window_width, int window_height)
    : window_width_(window_width), window_height_(window_height), window_(nullptr), renderer_(nullptr) {
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error("Error initializing SDL2: " + std::string(SDL_GetError()));
    }

    // Create the SDL2 window and renderer
    window_ = SDL_CreateWindow("Audio Waveform", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN);
    if (window_ == nullptr) {
        throw std::runtime_error("Error creating SDL2 window: " + std::string(SDL_GetError()));
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_ == nullptr) {
        throw std::runtime_error("Error creating SDL2 renderer: " + std::string(SDL_GetError()));
    }
}

AudioWaveform::~AudioWaveform() {
    // Clean up SDL2
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void AudioWaveform::drawWaveform(const int16_t* samples, int num_samples) {
    // Clear the screen
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer_);

    // Set the color for the waveform
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, SDL_ALPHA_OPAQUE);

    // Calculate the vertical scale factor to fit the waveform in the window
    float scale_factor = (float)window_height_ / ((float)32768 / 4);

    // Draw the waveform
    for (int i = 0; i < num_samples - 1; i++) {
        int x1 = (int)((float)i / (float)num_samples * window_width_);
        int y1 = (int)((float)samples[i] * scale_factor + window_height_ / 2);
        int x2 = (int)((float)(i + 1) / (float)num_samples * window_width_);
        int y2 = (int)((float)samples[i + 1] * scale_factor + window_height_ / 2);
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }

    // Update the window
    SDL_RenderPresent(renderer_);
}

bool AudioWaveform::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
                } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_q) {
                return false;
            }
        }
    }
    return true;
}
