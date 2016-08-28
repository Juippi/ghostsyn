#include "utils.hpp"
#include <algorithm>

void print_words(unsigned char *data, size_t bytes, size_t line_len) {
    if (bytes > 0) {
	for (size_t i = 0; i < (bytes - 1) / line_len + 1; ++i) {
	    for (size_t k = 0; k < std::min(line_len, bytes - i * line_len); ++k) {
		if (k % 4 == 0) {
		    fprintf(stderr, " - ");
		}
		fprintf(stderr, "%.2x ", static_cast<int>(data[i * line_len + k]));
	    }
	    fprintf(stderr, "\n");
	}
    }
}
