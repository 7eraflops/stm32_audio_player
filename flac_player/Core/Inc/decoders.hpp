#pragma once

#include <cstdint>
#include <fatfs.h>

#include "Bit_reader.hpp"

uint64_t decode_utf8(Bit_reader<FIL> &reader);

uint64_t decode_unary(Bit_reader<FIL> &reader);

int64_t decode_and_unfold_rice(uint8_t rice_parameter, Bit_reader<FIL> &reader);
