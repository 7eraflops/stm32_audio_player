#pragma once

#include <unordered_map>
#include <vector>
#include "fatfs.h"

#include "Bit_reader.hpp"
#include "Flac_constants.hpp"
#include "Flac_types.hpp"
#include "decoders.hpp"

class Flac
{
private:
    uint8_t m_channel_index{};
    uint64_t m_sample_count{};
    uint64_t m_frame_count{};
    Stream_info m_stream_info{};
    Frame_info m_frame_info{};
    Vorbis_comment m_vorbis_comment;
    FIL &m_flac_file;
    Bit_reader<FIL> m_reader;
    std::vector<buffer_sample_type> m_audio_buffer;

    // internal functions
    // decoding values from bit codes
    uint16_t decode_block_size(uint8_t block_size_code);
    uint32_t decode_sample_rate(uint8_t sample_rate_code);
    uint8_t decode_sample_size(uint8_t sample_size_code);
    // stream decoding functions that have to be used in a specific order and shouldn't be accessible to user
    void check_flac_marker();
    void read_metadata();
    void read_metadata_block_STREAMINFO();
    void read_metadata_block_PADDING();
    void read_metadata_block_APPLICATION();
    void read_metadata_block_SEEKTABLE();
    void read_metadata_block_VORBIS_COMMENT();
    void read_metadata_block_CUESHEET();
    void read_metadata_block_PICTURE();
    void decode_subframe(uint8_t bits_per_sample);
    void decode_subframe_fixed(uint8_t predictor_order, uint8_t bits_per_sample);
    void decode_subframe_lpc(uint8_t predictor_order, uint8_t bits_per_sample);
    void linear_prediction(uint8_t predictor_order, const int16_t *predictor_coefficients, int8_t qlp_shift);
    void decode_residuals(uint8_t predictor_order);

public:
    explicit Flac(FIL &flac_file) : m_flac_file(flac_file), m_reader(flac_file) {};

    // Getter functions
    const Stream_info &get_stream_info() { return m_stream_info; }
    const uint64_t &get_sample_count() {return m_sample_count; }
    const Frame_info &get_frame_info() { return m_frame_info; }
    const Vorbis_comment &get_vorbis_comment() { return m_vorbis_comment; }
    const Bit_reader<FIL> &get_reader() const { return m_reader; }
    const std::vector<buffer_sample_type> &get_audio_buffer() const { return m_audio_buffer; }

    // decoder interface
    void set_file(FIL &flac_file);
    void initialize();
    bool decode_frame();
};
