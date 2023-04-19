// AudioCaptureBase.cpp
#include "AudioCaptureBase.h"

AudioCaptureBase::AudioCaptureBase(std::string device_name, bool dummy_audio)
    : captureReady(false), buffer_(PingPongBuffer(4096))
{
    std::cout << "Initialising audio hardware..." << std::endl;
}

void AudioCaptureBase::register_callback(DataAvailableCallback cb)
{
    AudioCaptureBase::callbacks.push_back(cb);
}

void AudioCaptureBase::call_callbacks(const std::vector<short> &full_buffer, int buffer_index)
{
    for (auto cb : AudioCaptureBase::callbacks)
    {
        cb(full_buffer);
    }
}

bool AudioCaptureBase::quit = false;
std::vector<AudioCaptureBase::DataAvailableCallback> AudioCaptureBase::callbacks;