#include "Wav.hpp"

Wav::Wav(FIL& file, const Stream_info& stream_info) : file(file), stream_info(stream_info)
{
    encode_header(stream_info);
}

Wav::~Wav()
{
    f_sync(&file);
    f_close(&file);
}

void Wav::encode_header(const Stream_info& stream_info)
{
    UINT bytes_written;

    // Write RIFF header
    f_write(&file, "RIFF", 4, &bytes_written);
    auto data_size = stream_info.channels * stream_info.total_samples * 2; // 2 bytes per sample
    write_int32(data_size + 36);
    f_write(&file, "WAVE", 4, &bytes_written);

    // Write fmt chunk
    f_write(&file, "fmt ", 4, &bytes_written);
    write_int32(16);  // Chunk size for PCM
    write_int16(1);   // PCM format
    write_int16(stream_info.channels);
    write_int32(stream_info.sample_rate);
    auto frame_size = 2 * stream_info.channels;  // 2 bytes per sample
    auto byte_rate = stream_info.sample_rate * frame_size;
    write_int32(byte_rate);
    write_int16(frame_size);
    write_int16(16);  // Bits per sample

    // Write data chunk header
    f_write(&file, "data", 4, &bytes_written);
    write_int32(data_size);
}

void Wav::write_samples(const std::vector<buffer_sample_type>& buffer)
{
    for (const auto& sample : buffer)
    {
    	write_int16(static_cast<int16_t>(sample));
    }
}

void Wav::write_int32(int32_t value)
{
    UINT bytes_written;
    uint8_t data[4] = {
        static_cast<uint8_t>((value >> 0) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 24) & 0xFF)
    };
    f_write(&file, data, 4, &bytes_written);
}

void Wav::write_int16(int16_t value)
{
    UINT bytes_written;
    uint8_t data[2] = {
        static_cast<uint8_t>((value >> 0) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF)
    };
    f_write(&file, data, 2, &bytes_written);
}
