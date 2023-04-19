// AudioCaptureLinux.h
#ifdef __linux__
#pragma once
#include <alsa/asoundlib.h>
#include "AudioCaptureBase.h"

class AudioCaptureLinux : public AudioCaptureBase {
public:
    AudioCaptureLinux(std::string device_name, bool dummy_audio);
    ~AudioCaptureLinux();

    // Implement platform-specific methods
    std::string prompt_device_selection() override;
    void openDevice(const std::string &device_name) override;
    void closeDevice() override;
    void startCapture() override;
    void stopCapture() override;

private:
    // Platform-specific members
    snd_pcm_t *handle;
    snd_async_handler_t *pcm_callback;

    std::string device_name; // Add this line here

    // Platform-specific methods
    static void MyCallback(snd_async_handler_t *pcm_callback);
    void CaptureThreadFunction();
    void DummyAudioThreadFunction();
};
#endif
