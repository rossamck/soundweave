#include <alsa/asoundlib.h>
#include <iostream>

// Define the callback function
void callback(snd_async_handler_t* handler)
{
    int buffer_size = 1024;
    // Get the audio data buffer
    short* buffer = (short*)snd_async_handler_get_callback_private(handler);

    // Print the audio data to the console
    for (int i = 0; i < buffer_size; ++i)
    {
        std::cout << buffer[i] << " ";
    }
    std::cout << std::endl;

    // Read new data into the buffer
    snd_pcm_t* handle = snd_async_handler_get_pcm(handler);
    snd_pcm_sframes_t frames = snd_pcm_readi(handle, buffer, buffer_size);
    
    for (int i = 0; i < buffer_size; i++) {
        std::cout << buffer[i] << "test" << std::endl;
    }

    if (frames < 0)
    {
        // Handle error
        std::cerr << "Error reading from audio device: " << snd_strerror(frames) << std::endl;
    }
}

int main()
{
    // Open the audio device for recording
    snd_pcm_t* handle;
    int result = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (result < 0)
    {
        std::cerr << "Error opening audio device: " << snd_strerror(result) << std::endl;
        return -1;
    }

    // Set microphone parameters
    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
 
    unsigned int sample_rate = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, nullptr);
    snd_pcm_hw_params_set_channels(handle, params, 1);
    snd_pcm_hw_params(handle, params);

    // Allocate memory for audio data
    const int buffer_size = 1024;
    short* buffer = new short[buffer_size];

    // Add the callback function to the audio stream
    snd_async_handler_t* handler;
    snd_async_add_pcm_handler(&handler, handle, callback, buffer);
    const char* error = snd_strerror(-38);
    std::cout << "Error: " << error << std::endl;


    // Start recording
    snd_pcm_prepare(handle);
    snd_pcm_start(handle);

    // Wait for the recording to stop
    std::cin.get();

    // Clean up
    snd_pcm_drop(handle);
   
}