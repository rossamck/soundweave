#include "AudioCapture.h"
#include "PingPongBuffer.h"

#ifdef _WIN32
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#endif

bool AudioCapture::quit = false;
std::vector<AudioCapture::DataAvailableCallback> AudioCapture::callbacks;

AudioCapture::AudioCapture(std::string device_name, bool dummy_audio)
    : captureReady(false), callback(nullptr), buffer_(PingPongBuffer(4096)), dummy_audio(dummy_audio)
{
    std::cout << "Initialising audio hardware..." << std::endl;

    if (!dummy_audio)
    {
        // if device name has not been specified, prompt the user for it
        if (device_name.size() == 0)
            device_name = prompt_device_selection();

        // int err = snd_pcm_open(&handle, "plughw:0,7",SND_PCM_STREAM_CAPTURE,0);
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

        err = snd_async_add_pcm_handler(&pcm_callback, handle, &AudioCapture::MyCallback, this);
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

    // Initialize waveform data

    fftInputData.resize(4096);
    // doFFT = false;

    if (dummy_audio)
    {
        captureThread = std::thread(&AudioCapture::DummyAudioThreadFunction, this);
    }
    else
    {
        captureThread = std::thread(&AudioCapture::CaptureThreadFunction, this);
    }

    buffer_.set_on_buffer_full_callback(call_callbacks);
}

std::string AudioCapture::prompt_device_selection()
{
#ifdef _WIN32
    CoInitialize(NULL);
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDeviceCollection *pCollection = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&pEnumerator);
    if (FAILED(hr))
    {
        std::cerr << "Error creating IMMDeviceEnumerator: " << std::hex << hr << std::dec << std::endl;
        throw std::exception();
    }

    hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr))
    {
        std::cerr << "Error enumerating audio endpoints: " << std::hex << hr << std::dec << std::endl;
        pEnumerator->Release();
        throw std::exception();
    }

    UINT count;
    pCollection->GetCount(&count);

    std::vector<std::wstring> deviceNames(count);

    for (UINT i = 0; i < count; ++i)
    {
        IMMDevice *pDevice = NULL;
        LPWSTR pwszID = NULL;
        IPropertyStore *pProps = NULL;

        hr = pCollection->Item(i, &pDevice);
        if (FAILED(hr))
        {
            std::cerr << "Error getting device " << i << ": " << std::hex << hr << std::dec << std::endl;
            continue;
        }

        hr = pDevice->GetId(&pwszID);
        if (FAILED(hr))
        {
            std::cerr << "Error getting device ID: " << std::hex << hr << std::dec << std::endl;
            pDevice->Release();
            continue;
        }

        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        if (FAILED(hr))
        {
            std::cerr << "Error opening property store: " << std::hex << hr << std::dec << std::endl;
            CoTaskMemFree(pwszID);
            pDevice->Release();
            continue;
        }

        PROPVARIANT varName;
        PropVariantInit(&varName);
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        if (FAILED(hr))
        {
            std::cerr << "Error getting device friendly name: " << std::hex << hr << std::dec << std::endl;
            PropVariantClear(&varName);
            pProps->Release();
            CoTaskMemFree(pwszID);
            pDevice->Release();
            continue;
        }

        deviceNames[i] = varName.pwszVal;
        std::wcout << i << L". " << deviceNames[i] << std::endl;

        PropVariantClear(&varName);
        pProps->Release();
        CoTaskMemFree(pwszID);
        pDevice->Release();
    }

    std::cout << "Enter the index of the audio device to use: ";
    int device_index;
    std::cin >> device_index;

    if (device_index < 0 || device_index >= static_cast<int>(count))
    {
        std::cerr << "Invalid device index" << std::endl;
        pCollection->Release();
        pEnumerator->Release();
        CoUninitialize();
        throw std::exception();
    }

    std::wstring deviceNameW = deviceNames[device_index];
    std::string deviceName(deviceNameW.begin(), deviceNameW.end());

    pCollection->Release();
    pEnumerator->Release();
    CoUninitialize();

    return deviceName;

#else
    // ... (rest of the original Linux-specific code)
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
#endif
}

AudioCapture::~AudioCapture()
{
    std::cout << "Closing audiocapture" << std::endl;
    audioFile.close();
    snd_pcm_close(handle);
    fftInputData.clear();

    quit = true;
    captureCv.notify_one();
    if (captureThread.joinable())
    {
        captureThread.join();
    }
}

void AudioCapture::call_callbacks(const std::vector<short> &full_buffer, int buffer_index)
{
    for (auto cb : AudioCapture::callbacks)
    {
        cb(full_buffer);
    }
}

// Callback test
void AudioCapture::register_callback(DataAvailableCallback cb)
{
    AudioCapture::callbacks.push_back(cb);
}

void AudioCapture::MyCallback(snd_async_handler_t *pcm_callback)
{
    AudioCapture *instance = static_cast<AudioCapture *>(snd_async_handler_get_callback_private(pcm_callback));
    // std::cout << "alsa data available" << std::endl;
    std::unique_lock<std::mutex> lock(instance->captureMutex);
    instance->captureReady = true;
    lock.unlock();
    instance->captureCv.notify_one();
}

void AudioCapture::CaptureThreadFunction()

{

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

void AudioCapture::DummyAudioThreadFunction()
{
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
