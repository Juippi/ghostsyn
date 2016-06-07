#include "delay.hpp"

Delay::Delay() {
    buffers[0].resize(delay_length_max);
    buffers[1].resize(delay_length_max);
}

void Delay::run(double input[2], double output[2]) {
    for (size_t i = 0; i < 2; i++) {
	buffers[i][buffer_pos[i]] = buffers[i][buffer_pos[i]] * feedback + input[i];
	output[i] = buffers[i][buffer_pos[i]];
	if (++buffer_pos[i] >= delay[i]) {
	    buffer_pos[i] = 0;
	}
    }
}
