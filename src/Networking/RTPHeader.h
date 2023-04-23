#ifndef RTP_HEADER_H
#define RTP_HEADER_H

#include <cstdint>

struct RTPHeader {
    uint8_t version;
    uint8_t padding;
    uint8_t extension;
    uint8_t csrc_count;
    uint8_t marker;
    uint8_t payload_type;
    uint16_t seq_num;
    uint32_t timestamp;
    uint32_t ssrc;
};


#endif // RTP_HEADER_H