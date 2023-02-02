#include <iostream>
#include <alsa/asoundlib.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <fstream>

#include <SDL2/SDL.h>


int main(int argc, char *argv[]) {
    bool record = false;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "rec") == 0) {
        printf("Saving to file!");
        record = true;
        break;
        }
    }

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


    // Open a file to write the audio data
    std::ofstream audioFile("audio.raw", std::ios::binary);
    




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





    // Capture audio
    while (true) {
        snd_pcm_readi(handle, buffer, buffer_size);

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




        // Clear the screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        // Set the color for the waveform
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

        // Calculate the vertical scale factor to fit the waveform in the window
        float scale_factor = (float)600 / ((float)32768/4);
        
        int num_samples = buffer_size / sizeof(int16_t);;
        std::cout << num_samples << std::endl;

        // Draw the waveform
        for (int i = 0; i < num_samples - 1; i++) {
        int x1 = (int)((float)i / (float)num_samples * 800.0f);
        int y1 = (int)((float)buffer[i] * scale_factor + 300.0f);
        int x2 = (int)((float)(i + 1) / (float)num_samples * 800.0f);
        int y2 = (int)((float)buffer[i + 1] * scale_factor + 300.0f);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
        }

        // Update the window
        SDL_RenderPresent(renderer);



        if (record) {
            audioFile.write((char*) buffer, buffer_size * sizeof(short));
            for (int i = 0; i < buffer_size; i++) {
                std::cout << buffer[i] << std::endl;
        }
        }
    }

    // Clean up
    delete[] buffer;
    snd_pcm_close(handle);
    // Clean up SDL2
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();


    audioFile.close();

    return 0;
}