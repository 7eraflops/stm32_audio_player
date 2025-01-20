#pragma once

#include <cstdint>
#include "fatfs.h"

#include "Bit_reader.hpp"

// Function to decode a UTF-8 encoded number from a file stream (up to 5 bytes)
uint64_t decode_utf8(Bit_reader<FIL>& reader);

// Function to decode numbers encoded in unary code
uint64_t decode_unary(Bit_reader<FIL>& reader);

// Function to decode and unfold Rice coded and zig-zag folded numbers
int64_t decode_and_unfold_rice(uint8_t rice_parameter, Bit_reader<FIL>& reader);
