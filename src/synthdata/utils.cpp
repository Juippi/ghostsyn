#include "utils.hpp"
#include <cstring>
#include <stdexcept>

// Convert floating point value to byte array & truncate to N bits
std::vector<uint8_t> float2bin(const float value, unsigned int bits) {
    std::vector<uint8_t> bytes(sizeof(float));
    memcpy(bytes.data(), &value, sizeof(float));
    for (unsigned int i = 0; i < (32 - bits); ++i) {
        bytes[i / 8] &= ~(0x80 >> (i % 8));
    }
    return bytes;
}

// Append floating point value to byte array, but only N most sign. bytes
void bin_append_float(std::vector<uint8_t> &vec, const float value,
                      int trunc_bits, int bytes) {
    if (bytes < 0 || bytes > 4) {
        throw std::logic_error("invalid byte count in bin_append_float()");
    }
    auto fbin = float2bin(value, trunc_bits);
    size_t pos = vec.size();
    vec.resize(pos + bytes);
    memcpy(vec.data() + pos, fbin.data() + (4 - bytes), bytes);
}
