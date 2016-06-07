#include "compressor.hpp"
#include <cstdlib>
#include <cmath>

void Compressor::run(double input[2], double output[2]) {
    output[0] = input[0] * amp;
    output[1] = input[1] * amp;
    if ((fabs(output[0]) + fabs(output[1])) / 2 > threshold) {
	amp *= attack;
    } else {
	amp *= release;
	if (amp > 1.0) {
	    amp = 1.0;
	}
    }
    for (size_t i = 0; i < 2; i++) {
	if (output[i] > 1.0) {
	    output[i] = 1.0;
	} else if (output[i] < -1.0) {
	    output[i] = -1.0;
	}
    }
}
