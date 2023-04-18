#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <cmath>
#include <signal.h>
#include <climits>
#include <condition_variable>
#include <mutex>

#include <alsa/asoundlib.h>
#include <fftw3.h>

#include "PingPongBuffer.h"

/**
 * @class AudioCapture
 * @brief A wrapper around the asound library for capturing audio.
 */
class AudioCapture {
public:
    typedef void (*DataAvailableCallback)(const std::vector<short>&);

    /**
     * @brief Constructor for this class
     *
     * Prompts user for audio device if not provided. 
     * Gets handle for audio device and sets up necessary callbacks.
     * @param device_name The name of the audio device to capture from.
     */
    AudioCapture(std::string device_name);

    /**
     * @brief Registers callback for audio data
     *
     * Calls to this function is equivalent to subscribing to the data.
     * Multiple subscribers can be set up by making multiple calls.
     * @param cb A function pointer to the callback to be registered.
     */
    void register_callback(DataAvailableCallback cb);

    /**
     * @brief Prompts user to select audio device
     *
     * Lists all discoverable audio devices on system and prompts user to input a number selection.
     * @return The name of the selected audio device.
     */
    std::string prompt_device_selection();

    // Thread, mutex, and condition variable for the capture thread
    std::thread captureThread;
    std::mutex captureMutex;
    std::condition_variable captureCv;
    bool captureReady;
    
    /**
     * @brief Destructor for this class
     */
    ~AudioCapture();

private:

    /**
     * @brief Callback function used to handle audio originating from hardware 
     *
     * This function reads the available audio data into a PingPongBuffer.
     * Optionally, it renders a visualisation of the audio data.
     * @param pcm_callback A pointer to the ALSA asynchronous callback handler.
     */
    static void MyCallback(snd_async_handler_t* pcm_callback);

    /**
     * @brief Calls all callback functions with audio data when PingPongBuffer is full
     *
     * This function reads iterates through all callbacks registered with 'register_callback'.
     * Passes audio data from the PingPongBuffer, which is typically 4096 bytes.
     * @param full_buffer A vector containing the audio data.
     * @param size The number of elements in the audio data vector.
     */
    static void call_callbacks(const std::vector<short>& full_buffer, int size);

    /**
     * @brief Function to run the capturing loop in a separate thread
     */
    void CaptureThreadFunction();

    std::ofstream audioFile;
    snd_pcm_t* handle;
    snd_async_handler_t* pcm_callback;
    std::vector<float> waveform;
    static std::vector<DataAvailableCallback> callbacks;
    static bool quit;

    std::vector<short> fftInputData;

    std::vector<int> tempbuffer;
    DataAvailableCallback callback;

    PingPongBuffer buffer_;
};

#endif  // AUDIO_CAPTURE_H