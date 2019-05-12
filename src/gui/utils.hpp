#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <limits>
#include <vector>

// Print bytes
// data - ptr to data
// size - bytes of data
// line_len - no of bytes per line
void print_words(unsigned char *data, size_t bytes, size_t line_len);

// Convert string to integer, returning the positive value [lo, hi)
// on success, or -1 if conversion fails or out of range
template <typename T>
T atoi_pos(const std::string &str, T lo = 0, T hi = std::numeric_limits<T>::max()) {
    if (str.size() == 0) {
	return -1;
    }
    char *endptr;
    long long_val = strtol(str.c_str(), &endptr, 10);
    if (endptr != str.c_str() + str.size()) {
	return -1;
    }
    // if (long_val > std::numeric_limits<T>::max()) {
    // 	return -1;
    // }
    T val = static_cast<T>(long_val);
    if (val < lo || val >= hi) {
	return -1;
    }
    return val;
}

template <typename T>
std::string tostr(const T &val) {
    std::stringstream ss;
    ss << val;
    return ss.str();
}

std::vector<std::string> split(std::string &str, char sep = ' ');
