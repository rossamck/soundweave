#include "AudioCapture.h"
#include <iostream>

AudioCapture::AudioCapture(const std::string &device_name, bool use_default_device, bool dummy_audio)
    : device_name_(device_name), use_default_device_(use_default_device), dummy_audio_(dummy_audio)
{
    std::cout << "Starting capture class" << std::endl;
    if (!dummy_audio_ && dac_.getDeviceCount() < 1)
    {
        throw std::runtime_error("No audio devices found");
    }
}

AudioCapture::~AudioCapture()
{
    if (dac_.isStreamOpen())
        dac_.closeStream();
}

void AudioCapture::register_callback(AudioDataCallback callback)
{
    callback_ = callback;
}

int AudioCapture::RtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                                  double streamTime, RtAudioStreamStatus status, void *userData)
{
    AudioCapture *audio_capture = static_cast<AudioCapture *>(userData);
    // std::cout << "TEST YAY" << std::endl;

    if (status)
    {
        std::cout << "Stream overflow detected!" << std::endl;
    }

    if (audio_capture->callback_)
    {
        audio_capture->callback_(std::vector<short>(static_cast<short *>(inputBuffer),
                                                    static_cast<short *>(inputBuffer) + nBufferFrames));
    }

    return 0;
}

void AudioCapture::start()
{

    if (dummy_audio_)
    {
        generateSineWave();
    }
    else
    {
        RtAudio::StreamParameters parameters;
        if (use_default_device_)
        {
            parameters.deviceId = dac_.getDefaultInputDevice();
        }
        else
        {
            parameters.deviceId = getDeviceId(device_name_);
        }

        parameters.nChannels = 1;
        parameters.firstChannel = 0;
        unsigned int sample_rate = 44100;
        unsigned int buffer_frames = 256;

        RtAudioFormat format = RTAUDIO_SINT16;

        if (dac_.openStream(nullptr, &parameters, format, sample_rate, &buffer_frames, &AudioCapture::RtAudioCallback, this))
        {
            throw std::runtime_error(dac_.getErrorText());
        }

        if (dac_.startStream())
        {
            throw std::runtime_error(dac_.getErrorText());
        }
    }
}

void AudioCapture::stop()
{
    if (dac_.isStreamRunning())
    {
        dac_.stopStream();
    }
}

unsigned int AudioCapture::getDeviceId(const std::string &device_name)
{
    // Find the device ID by name
    unsigned int device_count = dac_.getDeviceCount();
    for (unsigned int i = 0; i < device_count; i++)
    {
        RtAudio::DeviceInfo info = dac_.getDeviceInfo(i);
        if (info.name == device_name)
        {
            return i;
        }
    }
    throw std::runtime_error("Device not found");
}


void AudioCapture::generateSineWave() {
    const int sample_rate = 44100;
    const double frequency = 440.0; // 440 Hz (A4 note)
    const double two_pi = 2.0 * M_PI;
    const double increment = frequency * two_pi / sample_rate;
    double phase = 0;
    const double amplitude_scaling_factor = 0.1; // Adjust this value as needed to fit the display


    while (true) {
        std::vector<short> buffer(256);
        for (size_t i = 0; i < buffer.size(); i++) {
            buffer[i] = static_cast<short>(std::numeric_limits<short>::max()/8 * std::sin(phase));
            phase += increment;
            if (phase >= two_pi) {
                phase -= two_pi;
            }
        }

        if (callback_) {
            callback_(buffer);
        }
    }
}