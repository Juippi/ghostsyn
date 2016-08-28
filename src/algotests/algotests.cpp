#include <fstream>
#include <iostream>
#include <vector>
#include <sndfile.h>

int main(int argc, char *argv[]) {

    if (argc != 2) {
	std::cerr << argv[0] << " <outifile> " << std::endl;
	return 1;
    }

    std::vector<float> outbuf(44100 * 60);

    SF_INFO info;
    info.samplerate = 44100;
    info.channels = 1;
    info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    for (int a = 0; a < 10; ++a) {

	if (a % 2 == 0) {
	    // naive saw
	    float val = 0;

	    for (int i = 0; i < 44100; ++i) {
		val += 0.02;
		if (val > 0.5) {
		    val -= 1.0;
		}

		outbuf[a * 44100 + i] = val * 0.1;
	    }
	} else {
	    // integrating saw
	    float val = 0;

	    float prev0 = 0;
	    float prev1 = 0;

	    for (int i = 0; i < 44100; ++i) {
		val += 0.02;
		if (val > 1.0) { // TODO: need to adjust C after 0.5 -> 1.0 change
		    val -= 2.0;
		}

		float d0 = val * val * val - val / 4;
		float out0 = d0 - prev0;
		prev0 = d0;

		float out1 = out0 - prev1;
		prev1 = out0;

		std::cout << val << " " << out1 << std::endl;

		// TODO: do the math for vol corr
 		outbuf[a * 44100 + i] = out1 * out1 * 150;
	    }
	}
    }

    SNDFILE *outfile = sf_open(argv[1], SFM_WRITE, &info);
    sf_writef_float(outfile, outbuf.data(), 44100 * 60);
    sf_close(outfile);

    return 0;
}
