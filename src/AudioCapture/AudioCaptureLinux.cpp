// AudioCaptureLinux.cpp
#ifdef __linux__
#include "AudioCaptureLinux.h"

AudioCaptureLinux::AudioCaptureLinux(std::string device_name, bool dummy_audio)
    : AudioCaptureBase(device_name, dummy_audio), handle(nullptr), pcm_callback(nullptr)
{
    if (!dummy_audio)
    {
        // if device name has not been specified, prompt the user for it
 if (this->device_name.size() == 0)
    this->device_name = prompt_device_selection();


        openDevice(this->device_name);
        startCapture();
    }

    if (dummy_audio)  
    {      
    captureThread = std::thread(&AudioCaptureLinux::DummyAudioThreadFunction, this);
    }
    else
    {
        captureThread = std::thread(&AudioCaptureLinux::CaptureThreadFunction, this);
    }

    buffer_.set_on_buffer_full_callback(call_callbacks);
}

AudioCaptureLinux::~AudioCaptureLinux()
{
    std::cout << "Closing audiocapture" << std::endl;
    closeDevice();

    fftInputData.clear();

    quit = true;
    captureCv.notify_one();
    if (captureThread.joinable())
    {
        captureThread.join();
    }
}

std::string AudioCaptureLinux::prompt_device_selection()
{
    // Implementation of prompt_device_selection for Linux
    int device_index = 0;

    // Get a list of available audio devices
    void **hints;
    if (snd_device_name_hint(-1, "pcm", &hints) != 0)
    {
        std::cerr << "Error getting audio device hints" << std::endl;
        throw std::exception();
    }
    int i = 0;
    // Print the list of available audio devices

    for (void **hint = hints; *hint; hint++)
    {
        char *name = snd_device_name_get_hint(*hint, "NAME");
        char *desc = snd_device_name_get_hint(*hint, "DESC");
        std::cout << i++ << ". " << name << " - " << desc << std::endl;
        free(name);
        free(desc);
    }

    // Prompt the user to select an audio device

    std::cout << "Enter the index of the audio device to use: ";
    std::cin >> device_index;

    // Get the name of the selected audio device
    i = 0;
    char *name;

    for (void **hint = hints; *hint; hint++)
    {
        if (i++ == device_index)
        {
            name = snd_device_name_get_hint(*hint, "NAME");
            // device_name = name;
            // free(name);
            break;
        }
    }
    snd_device_name_free_hint(hints);
    std::string device_name(name);
    free(name);
    return device_name;
}

void AudioCaptureLinux::openDevice(const std::string &device_name)
{
    // Open the PCM device
    int err = snd_pcm_open(&handle, device_name.c_str(), SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0)
    {
        std::cerr << "Error opening PCM device: " << snd_strerror(err) << std::endl;
        throw std::runtime_error("Failed to open PCM device");
    }

    // Setup parameters
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    unsigned int sample_rate = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, nullptr);
    snd_pcm_hw_params_set_channels(handle, params, 1);
    snd_pcm_hw_params(handle, params);
}


void AudioCaptureLinux::closeDevice()
{
    // Implementation of closeDevice for Linux
        snd_pcm_close(handle);

}

void AudioCaptureLinux::startCapture()
{
    int err = snd_async_add_pcm_handler(&pcm_callback, handle, &AudioCaptureLinux::MyCallback, this);
    if (err < 0)
    {
        std::cerr << "Error setting PCM async handler: " << snd_strerror(err) << std::endl;
        throw std::runtime_error("Failed to set PCM async handler");
    }

    err = snd_pcm_start(handle);
    if (err < 0)
    {
        std::cerr << "Error starting PCM device: " << snd_strerror(err) << std::endl;
        throw std::runtime_error("Failed to start PCM device");
    }
}

void AudioCaptureLinux::stopCapture()
{
    // Implementation of stopCapture for Linux
        snd_pcm_drop(handle);

}

void AudioCaptureLinux::MyCallback(snd_async_handler_t *pcm_callback)
{
    // Implementation of MyCallback for Linux
    AudioCaptureLinux *instance = static_cast<AudioCaptureLinux *>(snd_async_handler_get_callback_private(pcm_callback));
    // std::cout << "alsa data available" << std::endl;
    std::unique_lock<std::mutex> lock(instance->captureMutex);
    instance->captureReady = true;
    lock.unlock();
    instance->captureCv.notify_one();
}

void AudioCaptureLinux::CaptureThreadFunction()
{
    // Implementation of CaptureThreadFunction for Linux
    while (!quit)
    {
        // std::cout << "alsa data processing in new thread" << std::endl;
        std::unique_lock<std::mutex> lock(captureMutex);
        captureCv.wait(lock, [this]
                       { return captureReady; });

        snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
        if (avail < 0)
        {
            std::cerr << "Error in snd_pcm_avail_update: " << snd_strerror(avail) << std::endl;
            return;
        }

        // std::cout << "avail = " << avail << std::endl;

        // Create a vector to store the audio data
        std::vector<short> buffer(avail);

        snd_pcm_sframes_t frames = snd_pcm_readi(handle, buffer.data(), avail);

        // Number of samples is frames * channels
        if (frames < 0)
        {
            std::cerr << "Error in snd_pcm_readi: " << snd_strerror(frames) << std::endl;
            if (frames == -EPIPE)
            {
                std::cerr << "Attempting to recover from broken pipe" << std::endl;
                int err = snd_pcm_prepare(handle);
                if (err < 0)
                {
                    std::cerr << "Error preparing PCM device: " << snd_strerror(err) << std::endl;
                    throw std::runtime_error("Failed to prepare PCM device");
                }
            }
            return;
        }

        if (buffer.size() > 2048)
        {
            std::cerr << "Buffer overflow" << std::endl;
            // return;
        }

        buffer_.add_data(buffer);
        buffer.clear();

        captureReady = false;
    }
}

void AudioCaptureLinux::DummyAudioThreadFunction()
{
    // Implementation of DummyAudioThreadFunction for Linux
     const int sample_rate = 44100;
    const double frequency = 440.0;
    const double amplitude = std::numeric_limits<short>::max() / 16.0;
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
        std::chrono::duration<double> sleep_duration(buffer.size() / static_cast<double>(sample_rate));
        std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration));
    }
}
#endif