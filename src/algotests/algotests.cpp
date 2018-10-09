#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/range/irange.hpp>

#include <sndfile.h>

using boost::irange;

void reverb(std::vector<float> &in, std::vector<float> &out) {
    auto len = std::min(in.size(), out.size());

    std::vector<float> delayline(8192);
    size_t delay_pos = 0;
    float feedback = 0.95;
    float dry = 0.2;
    float wet = 0.8;
    
    for (auto i : irange(0u, len)) {
	float d = in[i];
	    
	d += delayline[delay_pos] * feedback * 0.25;
	d -= delayline[(delay_pos + 673) % delayline.size()] * feedback * 0.25;
	d += delayline[(delay_pos + 2937) % delayline.size()] * feedback * 0.25;
	d -= delayline[(delay_pos + 3717) % delayline.size()] * feedback * 0.25;
	
	delayline[delay_pos] = -d;

	out[i] = in[i] * dry + d * wet;

	++delay_pos;
	delay_pos %= delayline.size();
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
	base[i] = ((i % 256) / 256.0f - 0.5f) * 0.3;
    }

    for (auto i : irange(15000u, 25000u)) {
	base[i] = ((i % 128) / 128.0f - 0.5f) * 0.3;
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
