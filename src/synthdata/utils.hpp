#pragma once
#include <cstdint>
#include <vector>

std::vector<uint8_t> float2bin(const float value, unsigned int bits = 24);
void bin_append_float(std::vector<uint8_t> &vec, const float value,
                      int trunc_bits, int bytes);
