#include "song_params.h"
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

void mastering(float *buf, size_t n_samples) {
    const float limiter_max = 0.995;

    const float preamp = 2.3;

    const float comp_th = 0.9;
    const float comp_att = 0.025;
    const float comp_rel = 0.015;

    const size_t lofi_end = 8200 * 48 * 6;
    const size_t lofi_fade_start = 8200 * 48 * 4;

    float comp_gain = 1.0;

    for (size_t i = 0; i < n_samples; ++i) {
	float in = buf[i] * preamp;

	if (i < lofi_end) {
	    int x;
	    if (i < lofi_fade_start) {
		x = 20;
		in *= 0.5;
	    } else {
		x = 20 + ((i - lofi_fade_start) * 64 / (lofi_end - lofi_fade_start));
		in *= (0.5 + ((i - lofi_fade_start) * 0.5 / (lofi_end - lofi_fade_start)));
	    }
	    int val = in * x;
	    in = val / static_cast<float>(x);
	}

	float out = in * comp_gain;

	if (fabs(out) > comp_th && comp_gain > comp_att) {
	    comp_gain -= comp_att;
	} else if (comp_gain < 1.0f) {
	    comp_gain = comp_gain + (1.0f - comp_gain) * comp_rel;
	}

	if (out > limiter_max) {
	    out = limiter_max;
	} else if (out < -limiter_max) {
	    out = -limiter_max;
	}

	buf[i] = out;
    }
}

int main(int argc, char *argv[]) {
    int song_frames = SONG_FRAMES;
    if (argc < 2) {
	std::cerr << argv[0] << " <outfile>" << std::endl;
	return 1;
    }
    if (argc > 2) {
	song_frames = std::min(atoi(argv[2]), song_frames);
    }

    std::vector<float> outbuf(song_frames);
    std::cerr << "calling synth for " << song_frames << " frames" << std::endl;
    synth(outbuf.data(), song_frames);
    //mastering(outbuf.data(), song_frames);

    SF_INFO info;
    info.samplerate = 44100;
    info.channels = 2;
    info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE *outfile = sf_open(argv[1], SFM_WRITE, &info);
    sf_writef_float(outfile, outbuf.data(), song_frames / 2);
    sf_close(outfile);

    float max = 0;
    for (auto &v : outbuf) {
	if (fabs(v) > max) {
	    max = fabs(v);
	}
    }
    std::cerr << "max: " << max << std::endl;

    return 0;
}
