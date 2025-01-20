#include "Flac.hpp"

void Flac::set_file(FIL &flac_file)
{
	m_flac_file = flac_file;
	m_reader.set_file(flac_file);
}

void Flac::initialize()
{
    check_flac_marker();
    read_metadata();
}

void Flac::check_flac_marker()
{
    if (m_reader.read_bits_unsigned(32) != Flac_constants::flac_marker)
    {
        return;
    }
}

void Flac::read_metadata()
{
    bool is_last_block = false;

    while (!is_last_block)
    {
        is_last_block = m_reader.read_bits_unsigned(1);
        block_type current_block_type = static_cast<block_type>(m_reader.read_bits_unsigned(7));
        uint32_t block_length = m_reader.read_bits_unsigned(24);

        switch (current_block_type)
        {
        case block_type::STREAMINFO:
            if (block_length != 34)
            {
                return;
            }
            read_metadata_block_STREAMINFO();
            break;
        case block_type::PADDING:
            m_reader.seek(block_length);
            break;
        case block_type::APPLICATION:
            // TODO: implement function for APPLICATION block
            m_reader.seek(block_length);
            break;
        case block_type::SEEKTABLE:
            // TODO: implement function for SEEKTABLE block
            m_reader.seek(block_length);
            break;
        case block_type::VORBIS_COMMENT:
            // read_metadata_block_VORBIS_COMMENT();
        	m_reader.seek(block_length);
            break;
        case block_type::CUESHEET:
            // TODO: implement function for CUESHEET block
            m_reader.seek(block_length);
            break;
        case block_type::PICTURE:
            // TODO: implement function for PICTURE block
            m_reader.seek(block_length);
            break;
        default:
            return;
            break;
        }
    }
}

void Flac::read_metadata_block_STREAMINFO()
{
    m_stream_info.min_block_size = m_reader.read_bits_unsigned(16);
    m_stream_info.max_block_size = m_reader.read_bits_unsigned(16);
    m_stream_info.min_frame_size = m_reader.read_bits_unsigned(24);
    m_stream_info.max_frame_size = m_reader.read_bits_unsigned(24);
    m_stream_info.sample_rate = m_reader.read_bits_unsigned(20);
    m_stream_info.channels = m_reader.read_bits_unsigned(3) + 1;
    m_stream_info.bits_per_sample = m_reader.read_bits_unsigned(5) + 1;
    m_stream_info.total_samples = m_reader.read_bits_unsigned(36);

    m_reader.seek(16); // skipping 16 bytes (md5 signature)
}

void Flac::read_metadata_block_VORBIS_COMMENT()
{
    uint32_t vendor_length{};
    for (int i = 0; i < 4; i++)
    {
        vendor_length = (vendor_length << 8) | m_reader.read_bits_unsigned(8);
    }

    std::vector<char> vendor_data(vendor_length);
    for (uint32_t i = 0; i < vendor_length; i++)
    {
        vendor_data[i] = static_cast<char>(m_reader.read_bits_unsigned(8));
    }
    m_vorbis_comment.vendor_string = std::string(vendor_data.begin(), vendor_data.end());

    uint32_t user_comment_count{};
    for (int i = 0; i < 4; i++)
    {
        user_comment_count = (user_comment_count << 8) | m_reader.read_bits_unsigned(8);
    }

    m_vorbis_comment.user_comments.clear();
    for (uint32_t i = 0; i < user_comment_count; i++)
    {
        uint32_t comment_length{};
        for (int j = 0; j < 4; j++)
        {
            comment_length = (comment_length << 8) | m_reader.read_bits_unsigned(8);
        }

        std::vector<char> comment_data(comment_length);
                for (uint32_t j = 0; j < comment_length; j++) {
                    comment_data[j] = static_cast<char>(m_reader.read_bits_unsigned(8));
                }
                std::string comment(comment_data.begin(), comment_data.end());

        size_t delimiter_pos = comment.find('=');
        if (delimiter_pos != std::string::npos)
        {
            std::string key = comment.substr(0, delimiter_pos);
            std::string value = comment.substr(delimiter_pos + 1);
            m_vorbis_comment.user_comments[key] = value;
        }
    }
}

bool Flac::decode_frame()
{
    if (m_reader.eos())
    {
        return 1;
    }

    if (m_reader.read_bits_unsigned(14) != Flac_constants::frame_sync_code)
    {
        return 1;
    }
    if (m_reader.read_bits_unsigned(1))
    {
        return 1;
    }

    m_frame_info.blocking_strategy = m_reader.read_bits_unsigned(1);
    uint8_t block_size_code = m_reader.read_bits_unsigned(4);
    uint8_t sample_rate_code = m_reader.read_bits_unsigned(4);
    m_frame_info.channel_assignment = m_reader.read_bits_unsigned(4);
    m_frame_info.bits_per_sample = decode_sample_size(m_reader.read_bits_unsigned(3));

    if (m_reader.read_bits_unsigned(1))
    {
        return 1;
    }

    m_frame_info.frame_or_sample_number = decode_utf8(m_reader);

    m_frame_info.block_size = decode_block_size(block_size_code);
    m_frame_info.sample_rate = decode_sample_rate(sample_rate_code);

    m_frame_info.crc_8 = m_reader.read_bits_unsigned(8);

    m_audio_buffer.resize(m_stream_info.channels * m_frame_info.block_size);

    if (m_frame_info.channel_assignment <= 0b0111)
    {
        for (m_channel_index = 0; m_channel_index < m_stream_info.channels; m_channel_index++)
        {
            decode_subframe(m_frame_info.bits_per_sample);
        }
    }
    else if (m_frame_info.channel_assignment <= 0b1010)
    {
        m_channel_index = 0;
        decode_subframe(m_frame_info.bits_per_sample + ((m_frame_info.channel_assignment == 0b1001) ? 1 : 0));

        m_channel_index = 1;
        decode_subframe(m_frame_info.bits_per_sample + ((m_frame_info.channel_assignment == 0b1001) ? 0 : 1));

        if (m_frame_info.channel_assignment == 8)
        {
            for (uint16_t i = 0; i < 2 * m_frame_info.block_size; i += 2)
            {
                m_audio_buffer[i + 1] = m_audio_buffer[i] - m_audio_buffer[i + 1];
            }
        }
        else if (m_frame_info.channel_assignment == 9)
        {
            for (uint16_t i = 0; i < 2 * m_frame_info.block_size; i += 2)
            {
                m_audio_buffer[i] += m_audio_buffer[i + 1];
            }
        }
        else if (m_frame_info.channel_assignment == 10)
        {
            int64_t mid{};
            for (uint16_t i = 0; i < 2 * m_frame_info.block_size; i += 2)
            {
                mid = (uint64_t)m_audio_buffer[i] << 1;
                mid |= m_audio_buffer[i + 1] & 1;
                m_audio_buffer[i] = (mid + m_audio_buffer[i + 1]) >> 1;
                m_audio_buffer[i + 1] = (mid - m_audio_buffer[i + 1]) >> 1;
            }
        }
    }

    /*
    for (size_t i = 0; i < m_audio_buffer.size(); i++)
    {
        m_audio_buffer[i] = m_audio_buffer[i] << (32 - m_frame_info.bits_per_sample);
    }
    */

    m_sample_count += m_frame_info.block_size;
    m_frame_count++;
    m_reader.align_to_byte();
    m_frame_info.crc_16 = m_reader.read_bits_unsigned(16);
    return 0;
}

void Flac::decode_subframe(uint8_t bits_per_sample)
{
    if (m_reader.read_bits_unsigned(1) != 0)
    {
        return;
    }

    uint8_t subframe_type_code = m_reader.read_bits_unsigned(6);
    if ((subframe_type_code >= 2 && subframe_type_code <= 7) ||
        (subframe_type_code >= 16 && subframe_type_code <= 31))
    {
        return;
    }

    uint8_t wasted_bits_per_sample{};
    if (m_reader.read_bits_unsigned(1))
    {
        wasted_bits_per_sample = static_cast<uint8_t>(decode_unary(m_reader)) + 1;
        bits_per_sample -= wasted_bits_per_sample;
    }

    uint8_t predictor_order{};

    if (subframe_type_code == 0b000000)
    {
        buffer_sample_type value = m_reader.read_bits_unsigned(bits_per_sample);
        for (uint16_t i = 0; i < m_stream_info.channels * m_frame_info.block_size; i += m_stream_info.channels)
        {
            m_audio_buffer[i + m_channel_index] = value;
        }
    }
    else if (subframe_type_code == 0b000001)
    {
        for (uint16_t i = 0; i < m_stream_info.channels * m_frame_info.block_size; i += m_stream_info.channels)
        {
            m_audio_buffer[i + m_channel_index] = m_reader.read_bits_signed(bits_per_sample);
        }
    }
    else if ((subframe_type_code & 0b111000) == 0b001000)
    {
        predictor_order = subframe_type_code & 0b000111;
        if (predictor_order > 4)
        {
            return;
        }
        decode_subframe_fixed(predictor_order, bits_per_sample);
    }
    else if ((subframe_type_code & 0b100000) == 0b100000)
    {
        predictor_order = (subframe_type_code & 0b011111) + 1;
        decode_subframe_lpc(predictor_order, bits_per_sample);
    }
    else
    {
        return;
    }
    if (wasted_bits_per_sample > 0)
    {
        for (uint16_t i = 0; i < m_audio_buffer.size(); i += m_stream_info.channels)
        {
            m_audio_buffer[i + m_channel_index] <<= wasted_bits_per_sample;
        }
    }
}

void Flac::decode_subframe_fixed(uint8_t predictor_order, uint8_t bits_per_sample)
{
    for (uint8_t i = 0; i < m_stream_info.channels * predictor_order; i += m_stream_info.channels)
    {
        m_audio_buffer[i + m_channel_index] = m_reader.read_bits_signed(bits_per_sample);
    }
    decode_residuals(predictor_order);

    linear_prediction(predictor_order, Flac_constants::fixed_prediction_coefficients[predictor_order], 0);
}

void Flac::decode_subframe_lpc(uint8_t predictor_order, uint8_t bits_per_sample)
{
    for (uint8_t i = 0; i < m_stream_info.channels * predictor_order; i += m_stream_info.channels)
    {
        m_audio_buffer[i + m_channel_index] = m_reader.read_bits_signed(bits_per_sample);
    }

    uint8_t qlp_bit_precision = m_reader.read_bits_unsigned(4);
    if (qlp_bit_precision == 0b1111)
    {
        return;
    }
    qlp_bit_precision++;

    int8_t qlp_shift = m_reader.read_bits_signed(5);

    int16_t predictor_coefficients[32]{};
    for (uint8_t i = 0; i < predictor_order; i++)
    {
        predictor_coefficients[i] = m_reader.read_bits_signed(qlp_bit_precision);
    }

    decode_residuals(predictor_order);

    linear_prediction(predictor_order, predictor_coefficients, qlp_shift);
}

void Flac::linear_prediction(uint8_t predictor_order, const int16_t *predictor_coefficients, int8_t qlp_shift)
{
    for (uint16_t i = m_stream_info.channels * predictor_order; i < m_audio_buffer.size(); i += m_stream_info.channels)
    {
        int64_t prediction{};
        for (uint16_t j = 0; j < m_stream_info.channels * predictor_order; j += m_stream_info.channels)
        {
            prediction += m_audio_buffer[i - m_stream_info.channels - j + m_channel_index] * predictor_coefficients[j / m_stream_info.channels];
        }
        m_audio_buffer[i + m_channel_index] += (prediction >> qlp_shift);
    }
}

void Flac::decode_residuals(uint8_t predictor_order)
{
    uint8_t residual_coding_method = m_reader.read_bits_unsigned(2);
    if (residual_coding_method == 0b10 || residual_coding_method == 0b11)
    {
        return;
    }
    uint8_t parameter_bit_size = residual_coding_method == 0b00 ? 4 : 5;
    uint8_t rice_partition_order = m_reader.read_bits_unsigned(4);
    uint16_t rice_partition_count = 1 << rice_partition_order;
    uint16_t rice_partition_size = (m_frame_info.block_size) / rice_partition_count;

    uint8_t escape_code = (residual_coding_method == 0) ? 0xF : 0x1F;

    for (uint16_t i = 0; i < rice_partition_count; i++)
    {
        uint8_t rice_parameter = m_reader.read_bits_unsigned(parameter_bit_size);
        uint16_t start = (i * rice_partition_size + ((i == 0) ? predictor_order : 0));
        uint16_t end = ((i + 1) * rice_partition_size);

        if (rice_parameter != escape_code)
        {
            for (size_t j = m_stream_info.channels * start; j < m_stream_info.channels * end; j += m_stream_info.channels)
            {
                m_audio_buffer[j + m_channel_index] = decode_and_unfold_rice(rice_parameter, m_reader);
            }
        }
        else
        {
            uint8_t bit_count = m_reader.read_bits_unsigned(5);
            for (size_t j = m_stream_info.channels * start; j < m_stream_info.channels * end; j += m_stream_info.channels)
            {
                m_audio_buffer[j + m_channel_index] = m_reader.read_bits_signed(bit_count);
            }
        }
    }
}

uint16_t Flac::decode_block_size(uint8_t block_size_code)
{
    switch (block_size_code)
    {
    case 0b0110:
        return m_reader.read_bits_unsigned(8) + 1;

    case 0b0111:
        return m_reader.read_bits_unsigned(16) + 1;

    case 0b0000:
        return 1;

    default:
        break;
    }

    return Flac_constants::block_sizes[block_size_code];
}

uint32_t Flac::decode_sample_rate(uint8_t sample_rate_code)
{
    switch (sample_rate_code)
    {
    case 0b0000:
        return m_stream_info.sample_rate;

    case 0b1100:
        return m_reader.read_bits_unsigned(8) * 1000;

    case 0b1101:
        return m_reader.read_bits_unsigned(16);

    case 0b1110:
        return m_reader.read_bits_unsigned(16) * 10;

    case 0b1111:
        return 1;

    default:
        break;
    }

    return Flac_constants::sample_rates[sample_rate_code];
}

uint8_t Flac::decode_sample_size(uint8_t sample_size_code)
{
    switch (sample_size_code)
    {
    case 0b000:
        return m_stream_info.bits_per_sample;

    case 0b011:
        return 1;

    default:
        break;
    }

    return Flac_constants::bits_per_sample_table[sample_size_code];
}
