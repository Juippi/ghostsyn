#pragma once

extern "C" {

// current playback state returned by tracker-embed synth build
struct playback_state {
    uint32_t current_pattern;
    uint32_t current_row;
} __attribute__((packed));

__attribute__((regparm(3))) void synth(float *outbuf, int samples,
				       struct playback_state *state_out);

void synth_update(uint8_t *pattern_data, uint32_t pattern_data_bytes,
		  uint8_t *order_data, uint32_t order_data_bytes,
		  uint8_t *instrument_data, uint32_t instrument_data_bytes,
		  uint8_t *trigger_point_data, uint32_t trigger_points_data_len,
		  int module_count, int tracker_ticklen,
		  int num_rows,
		  // num_tracks * module_count elements, if corresponding value for
		  // element set, skip processing that module
		  uint8_t *module_skip_flags, uint32_t module_skip_flags_len);
};
