#pragma once
#include <cstdint>
#include <fatfs.h>

template <typename FatFS_File>
class Bit_reader
{
private:
    FatFS_File *m_file{};
    uint64_t m_bit_buffer{};
    uint8_t m_bits_in_buffer{};
    FSIZE_t m_file_size{};
    FSIZE_t m_current_pos{};

    static constexpr size_t BUFFER_SIZE = 32768;
    uint8_t m_read_buffer[BUFFER_SIZE]{};
    size_t m_buffer_pos{};
    size_t m_buffer_size{};

    bool fill_buffer()
    {
        if (m_file == nullptr || eos())
        {
            return false;
        }

        UINT bytes_read;
        FRESULT result = f_read(m_file, m_read_buffer, BUFFER_SIZE, &bytes_read);
        if (result != FR_OK)
        {
            return false;
        }

        m_buffer_size = bytes_read;
        m_buffer_pos = 0;
        return bytes_read > 0;
    }

public:
    explicit Bit_reader(FatFS_File &file) : m_file(&file)
    {
        m_file_size = f_size(m_file);
        m_current_pos = f_tell(m_file);
        fill_buffer();
    }

    void set_file(FIL &file)
    {
        m_file = &file;
        m_file_size = f_size(m_file);
        m_current_pos = f_tell(m_file);
        m_buffer_pos = 0;
        m_buffer_size = 0;
        m_bits_in_buffer = 0;
        m_bit_buffer = 0;
        fill_buffer();
    }

    bool eos() const
    {
        return m_current_pos >= m_file_size;
    }

    uint8_t get_byte()
    {
        if (m_file == nullptr || eos())
        {
            return 1;
        }

        if (m_buffer_pos >= m_buffer_size)
        {
            if (!fill_buffer())
            {
                return 1;
            }
        }

        uint8_t byte = m_read_buffer[m_buffer_pos++];
        m_current_pos++;
        return byte;
    }

    uint64_t read_bits_unsigned(uint8_t num_bits)
    {
        if (num_bits > 64)
        {
            return 1;
        }
        if (num_bits == 0)
        {
            return 0;
        }

        while (m_bits_in_buffer < num_bits)
        {
            uint8_t byte = get_byte();
            m_bit_buffer = (m_bit_buffer << 8) | byte;
            m_bits_in_buffer += 8;
        }

        uint64_t result = (m_bit_buffer >> (m_bits_in_buffer - num_bits)) & ((1ULL << num_bits) - 1);
        m_bits_in_buffer -= num_bits;
        return result;
    }

    int64_t read_bits_signed(uint8_t num_bits)
    {
        uint64_t result = read_bits_unsigned(num_bits);
        if (result & (1ULL << (num_bits - 1)))
        {
            return static_cast<int64_t>(result) - (1ULL << num_bits);
        }
        return static_cast<int64_t>(result);
    }

    void align_to_byte()
    {
        m_bits_in_buffer -= m_bits_in_buffer % 8;
    }

    bool peek_byte(uint8_t &byte) const
    {
        if (eos())
        {
            return false;
        }

        if (m_buffer_pos < m_buffer_size)
        {
            byte = m_read_buffer[m_buffer_pos];
            return true;
        }

        UINT bytes_read;
        FSIZE_t current = f_tell(m_file);
        FRESULT result = f_read(m_file, &byte, 1, &bytes_read);
        f_lseek(m_file, current);
        return (result == FR_OK && bytes_read == 1);
    }

    FSIZE_t tell() const
    {
        return m_current_pos;
    }

    FRESULT seek(FSIZE_t offset)
    {
        FSIZE_t new_pos = m_current_pos + offset;
        FSIZE_t buffer_start = m_current_pos - m_buffer_pos;
        FSIZE_t buffer_end = buffer_start + m_buffer_size;

        if (new_pos >= buffer_start && new_pos < buffer_end)
        {
            m_buffer_pos = new_pos - buffer_start;
            m_current_pos = new_pos;
            m_bits_in_buffer = 0;
            m_bit_buffer = 0;
            return FR_OK;
        }

        FRESULT result = f_lseek(m_file, new_pos);
        if (result == FR_OK)
        {
            m_current_pos = new_pos;
            m_buffer_pos = 0;
            m_buffer_size = 0;
            m_bits_in_buffer = 0;
            m_bit_buffer = 0;
            fill_buffer();
        }
        return result;
    }
};
