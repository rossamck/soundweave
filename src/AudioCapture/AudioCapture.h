#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include "RtAudio.h"
#include <string>
#include <vector>
#include <functional>

class AudioCapture {
public:
    using AudioDataCallback = std::function<void(const std::vector<short>&)>;

    AudioCapture(const std::string& device_name = "", bool use_default_device = true);
    ~AudioCapture();

    void register_callback(AudioDataCallback callback);
    void start();
    void stop();

private:
    static int RtAudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                               double streamTime, RtAudioStreamStatus status, void* userData);
    unsigned int getDeviceId(const std::string& device_name);

    RtAudio dac_;
    std::string device_name_;
    bool use_default_device_;
    AudioDataCallback callback_;
};

#endif // AUDIO_CAPTURE_H
