#include "RawAudioWriter.h"

RawAudioWriter::RawAudioWriter(const std::string &filename) : filename_(filename) {
    ofs_.open(filename_, std::ios::binary);
    if (!ofs_) {
        throw std::runtime_error("Error creating raw audio file");
    }
}

void RawAudioWriter::write_samples(const std::vector<int16_t> &samples) {
    ofs_.write(reinterpret_cast<const char *>(samples.data()), samples.size() * sizeof(int16_t));
}

RawAudioWriter::~RawAudioWriter() {
    ofs_.close();
}
