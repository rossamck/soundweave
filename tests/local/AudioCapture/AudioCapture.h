#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "../rtaudio/RtAudio.h"
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <limits>

class AudioCapture {
public:
    using AudioDataCallback = std::function<void(const std::vector<short>&)>;

    AudioCapture(const std::string& device_name, bool use_default_device, bool dummy_audio = false);
    ~AudioCapture();

    void register_callback(AudioDataCallback callback);
    void start();
    void stop();

private:
    static int RtAudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                               double streamTime, RtAudioStreamStatus status, void* userData);
    unsigned int getDeviceId(const std::string& device_name);
    void generateSineWave();

    RtAudio dac_;
    std::string device_name_;
    bool use_default_device_;
    bool dummy_audio_;
    AudioDataCallback callback_;
};

#endif // AUDIOCAPTURE_H