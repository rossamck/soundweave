#include "ClientManager.h"

void ClientManager::add_client(const ClientInfo &client_info)
{
    clients_.push_back(client_info);
    std::cout << "Added client with IP: " << client_info.target_ip
              << ", port: " << client_info.port << std::endl;

    udp::endpoint client_endpoint(
        boost::asio::ip::address::from_string(client_info.target_ip),
        client_info.port);
    boost::thread client_thread(
        &ClientManager::send_data_to_client, this, client_endpoint);
    client_threads_.push_back(boost::move(client_thread));
}

void ClientManager::send_data_to_all(const std::string &message)
{
    {
        std::unique_lock<std::mutex> lock(data_mutex_);
        message_ = message;
    }
    data_cv_.notify_all();
}

void ClientManager::send_audio_to_all(const std::vector<short> &audio_data)
{
    {
        std::unique_lock<std::mutex> lock(data_mutex_);
        audio_data_ = audio_data;
    }
    data_cv_.notify_all();
}


void ClientManager::send_data_to_client(const udp::endpoint &client_endpoint)
{
    uint16_t seq_num = 0;
    uint32_t ssrc = std::rand(); // Randomly generate SSRC

    while (!stop_threads_)
    {
        std::unique_lock<std::mutex> lock(data_mutex_);
        data_cv_.wait(lock);

        // Send text message
        if (!message_.empty()) {
            socket_.send_to(boost::asio::buffer(message_), client_endpoint);
            std::cout << "Sent message to client endpoint: " << client_endpoint
                      << std::endl;
            message_.clear();
        }

        // Send audio data
        if (!audio_data_.empty()) {
            uint32_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            RTPHeader rtp_header = {
                .version = 2,
                .padding = 0,
                .extension = 0,
                .csrc_count = 0,
                .marker = 0,
                .payload_type = 0, // Payload type 0 (PCMU)
                .seq_num = seq_num,
                .timestamp = timestamp,
                .ssrc = ssrc
            };
            std::vector<uint8_t> header = create_rtp_header(rtp_header);
            std::vector<uint8_t> rtp_packet(header.size() + audio_data_.size() * sizeof(short));
            std::memcpy(rtp_packet.data(), header.data(), header.size());
            std::memcpy(rtp_packet.data() + header.size(), audio_data_.data(), audio_data_.size() * sizeof(short));

            socket_.send_to(boost::asio::buffer(rtp_packet), client_endpoint);
            // std::cout << "Sent RTP audio data to client endpoint: " << client_endpoint
                    //   << std::endl;
            audio_data_.clear();
            seq_num++;
        }
    }
}


void ClientManager::print_clients()
{
    std::cout << "Connected clients:" << std::endl;
    for (const auto &client_info : clients_)
    {
        std::cout << "- " << client_info.target_ip << ":" << client_info.port
                  << std::endl;
    }
}



std::vector<uint8_t> ClientManager::create_rtp_header(const RTPHeader& rtp_header) {
    std::vector<uint8_t> header(12);

    header[0] = (rtp_header.version << 6) | (rtp_header.padding << 5) | (rtp_header.extension << 4) | rtp_header.csrc_count;
    header[1] = (rtp_header.marker << 7) | rtp_header.payload_type;
    header[2] = rtp_header.seq_num >> 8;
    header[3] = rtp_header.seq_num & 0xFF;
    header[4] = rtp_header.timestamp >> 24;
    header[5] = (rtp_header.timestamp >> 16) & 0xFF;
    header[6] = (rtp_header.timestamp >> 8) & 0xFF;
    header[7] = rtp_header.timestamp & 0xFF;
    header[8] = rtp_header.ssrc >> 24;
    header[9] = (rtp_header.ssrc >> 16) & 0xFF;
    header[10] = (rtp_header.ssrc >> 8) & 0xFF;
    header[11] = rtp_header.ssrc & 0xFF;

    return header;
}




void ClientManager::parse_received_data(const std::vector<uint8_t>& data, RTPHeader& rtp_header, std::vector<short>& audio_data) {
    if (data.size() < 12) {
        // Invalid packet, RTP header is at least 12 bytes
        return;
    }

    rtp_header.version = (data[0] >> 6) & 0x03;
    rtp_header.padding = (data[0] >> 5) & 0x01;
    rtp_header.extension = (data[0] >> 4) & 0x01;
    rtp_header.csrc_count = data[0] & 0x0F;
    rtp_header.marker = (data[1] >> 7) & 0x01;
    rtp_header.payload_type = data[1] & 0x7F;
    rtp_header.seq_num = (data[2] << 8) | data[3];
    rtp_header.timestamp = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    rtp_header.ssrc = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

    size_t audio_data_size = (data.size() - 12) / sizeof(short);
    audio_data.resize(audio_data_size);
    std::memcpy(audio_data.data(), data.data() + 12, audio_data_size * sizeof(short));
}
