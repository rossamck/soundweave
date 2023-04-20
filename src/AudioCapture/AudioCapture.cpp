#include "AudioCapture.h"
#include <iostream>

AudioCapture::AudioCapture(const std::string& device_name, bool use_default_device)
    : device_name_(device_name), use_default_device_(use_default_device) {
    std::cout << "Starting capture class" << std::endl;
    if (dac_.getDeviceCount() < 1) {
        throw std::runtime_error("No audio devices found");
    }
}

AudioCapture::~AudioCapture() {
    if (dac_.isStreamOpen()) dac_.closeStream();
}

void AudioCapture::register_callback(AudioDataCallback callback) {
    callback_ = callback;
}

int AudioCapture::RtAudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
                                   double streamTime, RtAudioStreamStatus status, void* userData) {
    AudioCapture* audio_capture = static_cast<AudioCapture*>(userData);
    // std::cout << "TEST YAY" << std::endl;

    if (status) {
        std::cout << "Stream overflow detected!" << std::endl;
    }

    if (audio_capture->callback_) {
        audio_capture->callback_(std::vector<short>(static_cast<short*>(inputBuffer),
                                                    static_cast<short*>(inputBuffer) + nBufferFrames));
    }

    return 0;
}

void AudioCapture::start() {
    RtAudio::StreamParameters parameters;
    if (use_default_device_) {
        parameters.deviceId = dac_.getDefaultInputDevice();
    } else {
        parameters.deviceId = getDeviceId(device_name_);
    }

    parameters.nChannels = 1;
    parameters.firstChannel = 0;
    unsigned int sample_rate = 44100;
    unsigned int buffer_frames = 256;

    RtAudioFormat format = RTAUDIO_SINT16;

    if (dac_.openStream(nullptr, &parameters, format, sample_rate, &buffer_frames, &AudioCapture::RtAudioCallback, this)) {
        throw std::runtime_error(dac_.getErrorText());
    }

    if (dac_.startStream()) {
        throw std::runtime_error(dac_.getErrorText());
    }
}

void AudioCapture::stop() {
    if (dac_.isStreamRunning()) {
        dac_.stopStream();
    }
}

unsigned int AudioCapture::getDeviceId(const std::string& device_name) {
    // Find the device ID by name
    unsigned int device_count = dac_.getDeviceCount();
    for (unsigned int i = 0; i < device_count; i++) {
        RtAudio::DeviceInfo info = dac_.getDeviceInfo(i);
        if (info.name == device_name) {
            return i;
        }
    }
    throw std::runtime_error("Device not found");
}
