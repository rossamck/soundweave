// AudioCaptureBase.h
#pragma once
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include "PingPongBuffer.h"
#include <cmath>

class AudioCaptureBase {
public:
    using DataAvailableCallback = std::function<void(const std::vector<short> &)>;
    virtual ~AudioCaptureBase() = default;

    // Common methods and members
    void register_callback(DataAvailableCallback cb);
    static void call_callbacks(const std::vector<short> &full_buffer, int buffer_index);

    // Platform-specific methods
    virtual std::string prompt_device_selection() = 0;
    virtual void openDevice(const std::string &device_name) = 0;
    virtual void closeDevice() = 0;
    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;

protected:
    AudioCaptureBase(std::string device_name, bool dummy_audio);

    PingPongBuffer buffer_;
    std::thread captureThread;
    std::vector<short> fftInputData;
    std::condition_variable captureCv;
    std::mutex captureMutex;
    bool captureReady;
    static bool quit;
    static std::vector<DataAvailableCallback> callbacks;
};