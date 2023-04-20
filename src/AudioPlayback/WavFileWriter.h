#include <cstdint>
#include <fstream>
#include <vector>
#include <string>

class WavFileWriter {
public:
    WavFileWriter(const std::string &filename, int sample_rate, int num_channels);
    void write_samples(const std::vector<int16_t> &samples);
    ~WavFileWriter();

private:
    std::string filename_;
    int sample_rate_;
    int num_channels_;
    size_t data_size_;

    void write_header();
    void update_header();
};
