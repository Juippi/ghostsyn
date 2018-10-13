#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <boost/range/irange.hpp>

#include <sndfile.h>

using boost::irange;

void reverb(std::vector<float> &in, std::vector<float> &out) {
    auto len = std::min(in.size(), out.size());

    std::vector<float> delayline(32768);
    const int num_taps = 16;
    size_t delay_pos = 0;
    float feedback = 0.96f / num_taps;
    float dry = 0.1f;
    float wet = 0.9f;
    float y1 = 0.0f;
    bool first = true;
    
    for (auto i : irange(0u, len)) {
	float d = in[i];
	float phase = 1.0f;
	int dtime = 0;
	
	for (auto i : irange(0, num_taps)) {
	    int magic = 19783;
	    dtime *= magic;
	    dtime += magic;
	    dtime %= delayline.size();
	    if (first) {
		std::cerr << dtime << std::endl;
	    }
	    d += delayline[(delay_pos + dtime) % delayline.size()] * feedback * phase;
	    phase *= -1.0;
	}

	// lp filt
	float din = y1 * 0.90f + d * 0.10f;
	y1 = din;

	delayline[delay_pos] = din;

	//out[i] = in[i] * dry + d * wet;
	out[i] = d;

	++delay_pos;
	delay_pos %= delayline.size();

	first = false;
    }
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
	std::cerr << argv[0] << " <outifile> " << std::endl;
	return 1;
    }

    static const int seconds = 60;
    static const int rate = 44100;
    static const int channels = 2;

    std::vector<float> base(rate * seconds);
    std::vector<float> outbuf(rate * seconds);

    for (auto i : irange(0u, 10000u)) {
	base[i] = ((i % 256) / 256.0f - 0.5f) * 0.8;
    }

    for (auto i : irange(15000u, 25000u)) {
	base[i] = ((i % 128) / 128.0f - 0.5f) * 0.8;
    }
    
    reverb(base, outbuf);

    SF_INFO info;
    info.samplerate = rate;
    info.channels = 1;
    info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE *outfile = sf_open(argv[1], SFM_WRITE, &info);
    sf_writef_float(outfile, outbuf.data(), rate * seconds);
    sf_close(outfile);

    return 0;
}
