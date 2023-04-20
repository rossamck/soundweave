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
    std::cout << "TEST YAY" << std::endl;

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

void AudioCapture::DummyAudioThreadFunction()
{
    const int sample_rate = 44100;
    const double frequency = 440.0;
    const double amplitude = std::numeric_limits<short>::max() / 2.0;
    double phase = 0.0;
    const double phase_increment = 2.0 * M_PI * frequency / sample_rate;

    while (!quit)
    {
        // Generate sine wave samples
        std::vector<short> buffer(4096);
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            buffer[i] = static_cast<short>(amplitude * std::sin(phase));
            phase += phase_increment;
            if (phase >= 2.0 * M_PI)
            {
                phase -= 2.0 * M_PI;
            }
        }

        buffer_.add_data(buffer);
        buffer.clear();

        // Sleep for the duration of the generated audio data
        // std::chrono::duration<double> sleep_duration(buffer.size() / static_cast<double>(sample_rate));
        // std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration));
    }
    throw std::runtime_error("Device not found");
}
