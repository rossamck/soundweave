// receive.cpp
#include "common.h"
#include "rtaudio/RtAudio.h"

#include <atomic>
#include <mutex>
#include <condition_variable>


std::vector<short> g_received_audio_data;
std::mutex g_audio_data_mutex;
std::condition_variable g_audio_data_cv;
std::atomic<bool> g_new_data_available(false);

void received_audio_data_callback(const std::vector<short> &audio_data)
{   
    std::cout << "Received audio data!" << std::endl;

    // Update the global buffer with the new audio data
    std::unique_lock<std::mutex> lock(g_audio_data_mutex);
    g_received_audio_data = audio_data;
    g_new_data_available.store(true);
    lock.unlock();
    g_audio_data_cv.notify_one();

    // Open the output file in append mode
    std::ofstream outfile("output.raw", std::ios::out | std::ios::app | std::ios::binary);

    // Check if the output file is open
    if (!outfile.is_open())
    {
        std::cerr << "Error opening output file" << std::endl;
        return;
    }

    // Write the received audio data to the file
    outfile.write(reinterpret_cast<const char *>(audio_data.data()), audio_data.size() * sizeof(short));

    // Close the output file
    outfile.close();
}


int rtaudio_callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
                     double streamTime, RtAudioStreamStatus status, void *userData)
{
    std::unique_lock<std::mutex> lock(g_audio_data_mutex);
    g_audio_data_cv.wait(lock, [] { return g_new_data_available.load(); });

    size_t dataSize = g_received_audio_data.size() * sizeof(short);
    std::memcpy(outputBuffer, g_received_audio_data.data(), dataSize);
    g_new_data_available.store(false);

    lock.unlock();

    return 0;
}

int main()
{
    std::cout << "Starting receiving client!" << std::endl;

    // Initialize the I/O context and UDP socket
    boost::asio::io_context io_context;
    unsigned short local_port = 13579;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), local_port));

    // Prepare a buffer and endpoint to receive incoming data
    char data[1024];
    udp::endpoint sender_endpoint;

   // Set up and start the RtAudio stream
    RtAudio dac;
    RtAudio::StreamParameters parameters;
    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = 1;
    parameters.firstChannel = 0;
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512;

    
    
        dac.openStream(&parameters, NULL, RTAUDIO_SINT16, sampleRate, &bufferFrames, &rtaudio_callback, NULL);
        dac.startStream();
  


    while (true)
    {
        // Receive audio data from the UDP socket
        std::size_t bytes_transferred = socket.receive_from(boost::asio::buffer(data), sender_endpoint);

        // Process the received audio data
        std::vector<short> received_audio_data(reinterpret_cast<short *>(data),
                                               reinterpret_cast<short *>(data) + bytes_transferred / sizeof(short));
        received_audio_data_callback(received_audio_data);
    }

    return 0;
}