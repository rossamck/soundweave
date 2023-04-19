#ifndef AUDIO_CAPTURE_MAC_H
#define AUDIO_CAPTURE_MAC_H

#if defined(__APPLE__) && defined(__MACH__)

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include "AudioCaptureBase.h"
#include <thread>
#include <chrono>

class AudioCaptureMac : public AudioCaptureBase
{
public:
    AudioCaptureMac(std::string device_name, bool dummy_audio);
    ~AudioCaptureMac();

private:
    void openDevice(const std::string &device_name);
    void closeDevice();
    void startCapture();
    void stopCapture();

    static OSStatus MyCallback(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumberFrames,
                               AudioBufferList *ioData);

    void CaptureThreadFunction();
    void DummyAudioThreadFunction();

    AudioUnit audioUnit;
    std::thread captureThread;
};

#endif // __APPLE__ && __MACH__

#endif // AUDIO_CAPTURE_MAC_H
