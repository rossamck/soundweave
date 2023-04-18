#include <iostream>
#include <alsa/asoundlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <chrono>

#include "Networking/holepunch.h"
#include "Networking/RTP.h"
#include "AudioCapture/AudioCapture.h"

// int main()
// {

//   std::cout << "Initialising..." << std::endl;
//   ipInformation otherClient;
//   otherClient = connectToClient();
//   std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
//   std::cout << "Socket is: " << otherClient.sock << std::endl;

//   const char *destination_ip = otherClient.ip;
//   int destination_port = otherClient.port;

//   // Parse command line arguments
//   // const char* destination_ip = "192.168.0.4";
//   // int destination_port = 12345;

//   // Open the microphone
//   snd_pcm_t *handle;
//   if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0) < 0)
//   {
//     std::cerr << "Error opening microphone" << std::endl;
//     return 1;
//   }

//   // Set microphone parameters
//   snd_pcm_hw_params_t *params;
//   snd_pcm_hw_params_alloca(&params);
//   snd_pcm_hw_params_any(handle, params);
//   snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
//   snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

//   unsigned int sample_rate = 44100;
//   snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, nullptr);
//   snd_pcm_hw_params_set_channels(handle, params, 1);
//   snd_pcm_hw_params(handle, params);

//   // Allocate memory for audio data
//   const int buffer_size = 1024; // reduce this
//   short *buffer = new short[buffer_size];

//   // Create the RTP object
//   RTP rtp(destination_ip, destination_port, 11); // check payload type!!!!

//   std::cout << "Local port = " << otherClient.own_port << std::endl;

//   // Capture audio and send it over RTP
//   while (true)
//   {
//     snd_pcm_readi(handle, buffer, buffer_size);
//     // std::cout << "Buffer size = " << buffer_size << std::endl;
//     // std::cout << "Sent size = " << buffer_size * sizeof(short) << std::endl;
//     // std::cout << "Buffer size usin sizeof(buffer) = " << sizeof(buffer) << std::endl;
//     rtp.SendPacket(buffer, buffer_size * sizeof(short), otherClient.sock);
//   }

//   // Clean up
//   delete[] buffer;
//   snd_pcm_close(handle);

//   return 0;
// }

#include <iostream>

void audio_data_callback(const std::vector<short>& audio_data) {
    // Print the number of samples in the audio data
    std::cout << "Received audio data with " << audio_data.size() << " samples" << std::endl;

    // You can process the audio data further here, e.g., save it to a file, analyze it, etc.
}


int main()
{
  std::cout << "Initialising..." << std::endl;
  ipInformation otherClient;
  otherClient = connectToClient();
  std::cout << "Main function: ip = " << otherClient.ip << " port = " << otherClient.port << " own port = " << otherClient.own_port << std::endl;
  std::cout << "Socket is: " << otherClient.sock << std::endl;

  const char *destination_ip = otherClient.ip;
  int destination_port = otherClient.port;

  AudioCapture audioCapture("default");
  audioCapture.register_callback(audio_data_callback);

  while (true) {

  }
  return 0;
}