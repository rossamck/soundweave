#include <cstdint>
#include <fstream>
#include <vector>
#include <string>

class RawAudioWriter {
public:
    RawAudioWriter(const std::string &filename);
    void write_samples(const std::vector<int16_t> &samples);
    ~RawAudioWriter();

private:
    std::string filename_;
    std::ofstream ofs_;
};
