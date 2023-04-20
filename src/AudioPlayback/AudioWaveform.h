#ifndef AUDIO_WAVEFORM_H
#define AUDIO_WAVEFORM_H

#include <SDL2/SDL.h>
#include <cstdint>

class AudioWaveform {
public:
    AudioWaveform(int window_width, int window_height);
    ~AudioWaveform();

    void drawWaveform(const int16_t* samples, int num_samples);
    bool processEvents();

private:
    int window_width_;
    int window_height_;
    SDL_Window* window_;
    SDL_Renderer* renderer_;
};

#endif // AUDIO_WAVEFORM_H
