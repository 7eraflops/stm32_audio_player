#ifndef WAV_HPP_
#define WAV_HPP_

#include <cstdint>
#include <fatfs.h>
#include <vector>

#include "Flac_types.hpp"

class Wav
{
public:
    Wav(FIL &file, const Stream_info &stream_info);
    ~Wav();
    void write_samples(const std::vector<buffer_sample_type> &buffer);

private:
    FIL &file;
    const Stream_info &stream_info;

    void encode_header(const Stream_info &stream_info);
    void write_int32(int32_t value);
    void write_int16(int16_t value);
};

#endif /* WAV_HPP_ */
