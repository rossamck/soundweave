#include "WavFileWriter.h"

WavFileWriter::WavFileWriter(const std::string &filename, int sample_rate, int num_channels)
    : filename_(filename), sample_rate_(sample_rate), num_channels_(num_channels), data_size_(0) {
    write_header();
}

void WavFileWriter::write_samples(const std::vector<int16_t> &samples) {
    std::ofstream ofs(filename_, std::ios::binary | std::ios::app);
    ofs.write(reinterpret_cast<const char *>(samples.data()), samples.size() * sizeof(int16_t));
    data_size_ += samples.size() * sizeof(int16_t);
    ofs.close();
}

WavFileWriter::~WavFileWriter() {
    update_header();
}

void WavFileWriter::write_header() {
    std::ofstream ofs(filename_, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Error creating WAV file");
    }

    // RIFF header
    ofs.write("RIFF", 4);
    ofs.write("\0\0\0\0", 4); // Placeholder for chunk size
    ofs.write("WAVE", 4);

    // Format chunk
    ofs.write("fmt ", 4);
    ofs.write("\20\0\0\0", 4); // Format chunk size: 16 bytes
    ofs.write("\1\0", 2); // Audio format: 1 (PCM)
    ofs.write(reinterpret_cast<const char *>(&num_channels_), 2);
    ofs.write(reinterpret_cast<const char *>(&sample_rate_), 4);
    int byte_rate = sample_rate_ * num_channels_ * sizeof(int16_t);
    ofs.write(reinterpret_cast<const char *>(&byte_rate), 4);
    int block_align = num_channels_ * sizeof(int16_t);
    ofs.write(reinterpret_cast<const char *>(&block_align), 2);
    ofs.write("\10\0", 2); // Bits per sample: 16

    // Data chunk
    ofs.write("data", 4);
    ofs.write("\0\0\0\0", 4); // Placeholder for data size

    ofs.close();
}

void WavFileWriter::update_header() {
    std::ofstream ofs(filename_, std::ios::binary | std::ios::in | std::ios::out);
    if (!ofs) {
        throw std::runtime_error("Error updating WAV file header");
    }

    // Update chunk size
    ofs.seekp(4, std::ios::beg);
    uint32_t chunk_size = 36 + data_size_;
    ofs.write(reinterpret_cast<const char *>(&chunk_size), 4);

    // Update data size
    ofs.seekp(40, std::ios::beg);
    ofs.write(reinterpret_cast<const char *>(&data_size_), 4);

    ofs.close();
}
