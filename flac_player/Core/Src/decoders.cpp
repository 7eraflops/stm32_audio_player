#include "decoders.hpp"

uint64_t decode_utf8(Bit_reader<FIL>& reader)
{
    uint8_t first_byte = reader.get_byte();

    static const struct
    {
        uint8_t mask;
        uint8_t match;
        uint8_t bits;
    } utf8_masks[] = {
        {0x80, 0x00, 0},
        {0xE0, 0xC0, 1},
        {0xF0, 0xE0, 2},
        {0xF8, 0xF0, 3},
        {0xFC, 0xF8, 4},
        {0xFE, 0xFC, 5},
        {0xFF, 0xFE, 6}
    };

    uint64_t code_point = 0;
    size_t additional_bytes = 0;

    for (const auto& mask : utf8_masks)
    {
        if ((first_byte & mask.mask) == mask.match)
        {
            code_point = first_byte & ~mask.mask;  // Strip the prefix bits
            additional_bytes = mask.bits;
            break;
        }
    }

    if (additional_bytes > 6)
    {
        return 1;
    }

    for (size_t i = 0; i < additional_bytes; i++)
    {
        uint8_t next_byte = reader.get_byte();

        if ((next_byte & 0xC0) != 0x80)
        {
            return 1;
        }

        code_point = (code_point << 6) | (next_byte & 0x3F);
    }

    return code_point;
}

uint64_t decode_unary(Bit_reader<FIL>& reader)
{
    uint64_t result = 0;

    while (reader.read_bits_unsigned(1) == 0)
    {
        result++;
    }
    return result;
}

int64_t decode_and_unfold_rice(uint8_t rice_parameter, Bit_reader<FIL>& reader)
{
    uint64_t quotient = decode_unary(reader);
    uint64_t remainder = reader.read_bits_unsigned(rice_parameter);

    uint64_t folded_rice = (quotient << rice_parameter) | remainder;
    if (folded_rice % 2 == 0)
    {
        int64_t unfolded_rice = static_cast<int64_t>(folded_rice >> 1);
        return unfolded_rice;
    }
    else
    {
        int64_t unfolded_rice = static_cast<int64_t>(~(folded_rice >> 1));
        return unfolded_rice;
    }
}
