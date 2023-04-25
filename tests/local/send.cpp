// send.cpp
#include "common.h"

void audio_data_callback(const std::vector<short> &audio_data, const udp::endpoint &server_endpoint, udp::socket &socket)
{
    // Send the captured audio data to the server
    socket.send_to(boost::asio::buffer(audio_data.data(), audio_data.size() * sizeof(short)), server_endpoint);
}



int main()
{
    std::cout << "Starting sending client" << std::endl;
    // Initialize the I/O context and UDP socket
    boost::asio::io_context io_context;
    udp::socket socket(io_context, udp::endpoint(udp::v4(), 0));

    // Set the server IP and port
    std::string server_ip = "192.168.0.164";
    unsigned short server_port = 13579;

    // Resolve the server endpoint
    udp::resolver resolver(io_context);
    udp::endpoint server_endpoint = *resolver.resolve(udp::v4(), server_ip, std::to_string(server_port)).begin();

    // Create an AudioCapture object and register the audio_data_callback
    AudioCapture audio_capture("", true, false);
    audio_capture.register_callback([&](const std::vector<short> &audio_data) {
        audio_data_callback(audio_data, server_endpoint, socket);
    });

    // Start capturing audio
    audio_capture.start();

    // Run the I/O context
    io_context.run();
    while (true) {}

    return 0;
}
