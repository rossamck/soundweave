// #include "../../../vcpkg/installed/x64-windows/include/rtaudio/RtAudio.h"


// Global variables and functions from the previous implementation
// ...
#include "common.h"
#include "rtaudio/RtAudio.h"
#include "AudioCapture/AudioCapture.h"
#include <atomic>
#include <mutex>
#include <condition_variable>

std::vector<short> g_received_audio_data;
std::mutex g_audio_data_mutex;
std::condition_variable g_audio_data_cv;
std::atomic<bool> g_new_data_available(false);

udp::endpoint remote_endpoint;
void audio_data_callback(const std::vector<short> &audio_data, const udp::endpoint &server_endpoint, udp::socket &socket)
{
    // Send the captured audio data to the server
    std::cout << "Sending audio!" << std::endl;
    socket.send_to(boost::asio::buffer(audio_data.data(), audio_data.size() * sizeof(short)), server_endpoint);
}

int inout(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
          double streamTime, RtAudioStreamStatus status, void *data)
{
    if (status)
        std::cout << "Stream over/underflow detected." << std::endl;

    // Send the input audio data over UDP
    std::vector<short> input_audio_data(reinterpret_cast<short *>(inputBuffer),
                                        reinterpret_cast<short *>(inputBuffer) + nBufferFrames);
    udp::socket &socket = *reinterpret_cast<udp::socket *>(data);
    audio_data_callback(input_audio_data, remote_endpoint, socket);

    // Play the received audio data
    std::unique_lock<std::mutex> lock(g_audio_data_mutex);
    g_audio_data_cv.wait(lock, [] { return g_new_data_available.load(); });

    size_t dataSize = g_received_audio_data.size() * sizeof(short);
    std::memcpy(outputBuffer, g_received_audio_data.data(), dataSize);
    g_new_data_available.store(false);

    lock.unlock();

    return 0;
}
    int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <local_port> <remote_ip> <remote_port>\n";
        return 1;
    }

    unsigned short local_port = static_cast<unsigned short>(std::stoi(argv[1]));
    std::string remote_ip = argv[2];
    unsigned short remote_port = static_cast<unsigned short>(std::stoi(argv[3]));

    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), local_port));

    // Resolve the remote endpoint
    udp::resolver resolver(io_context);
    remote_endpoint = *resolver.resolve(udp::v4(), remote_ip, std::to_string(remote_port)).begin();

    // Prepare a buffer and endpoint to receive incoming data
    char data[1024];
    udp::endpoint sender_endpoint;

    RtAudio adac;
    if (adac.getDeviceCount() < 1)
    {
        std::cout << "\nNo audio devices found!\n";
        exit(0);
    }

    unsigned int bufferFrames = 512;
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = adac.getDefaultInputDevice();
    iParams.nChannels = 1;
    oParams.deviceId = adac.getDefaultOutputDevice();
    oParams.nChannels = 1;

  
adac.openStream(&oParams, &iParams, RTAUDIO_SINT16, 44100, &bufferFrames, &inout, &socket);
        adac.startStream();

         AudioCapture audio_capture("", true, false);
  audio_capture.register_callback([&](const std::vector<short> &audio_data) {
        audio_data_callback(audio_data, remote_endpoint, socket);
    });
    while (true)
    {
        // Receive audio data from the UDP socket
        std::size_t bytes_transferred = socket.receive_from(boost::asio::buffer(data), sender_endpoint);

        // Process the received audio data
        std::vector<short> received_audio_data(reinterpret_cast<short *>(data),
                                               reinterpret_cast<short *>(data) + bytes_transferred / sizeof(short));
                // Update the global g_received_audio_data and set the g_new_data_available flag
        std::unique_lock<std::mutex> lock(g_audio_data_mutex);
        g_received_audio_data = received_audio_data;
        g_new_data_available.store(true);
        lock.unlock();
        g_audio_data_cv.notify_one();
    }



    return 0;
}

