#include <fstream>
#include <iostream>
#include <ctime>
#include <chrono>
#include <sndfile.h>
#include <string>
#include <algorithm>
#include <cctype>
#include "synth_api.h"
#include "tracker_data.hpp"

int main(int argc, char *argv[]) {

    if (argc != 5) {
	std::cerr << std::string(argv[0]) << " <-c|-r> <songfile> <asm out> <writer hdr out>"
		  << std::endl;
	return 1;
    }

    enum {MODE_COMPILE, MODE_RENDER} mode;
    if (std::string(argv[1]) == "-c") {
	mode = MODE_COMPILE;
    } else if (std::string(argv[1]) == "-c") {
	mode = MODE_RENDER;
    } else {
	std::cerr << "Must specify mode -c or -r as first arg" << std::endl;
	return 1;
    }

    std::string infile_name = argv[2];

    std::ifstream infile(infile_name);
    if (!infile.good()) {
	std::cerr << "Failed to open " << infile_name << std::endl;
	return 1;
    }

    if (mode == MODE_COMPILE) {
	try {
	    Json::Value json;
	    infile >> json;
	    TrackerData data(json);

	    std::ofstream asmout(argv[3]);

	    auto gen = std::chrono::system_clock::now();
	    std::time_t gen_time = std::chrono::system_clock::to_time_t(gen);

	    asmout << ";;; autogenerated from " << infile_name << std::endl;
	    asmout << ";;; " << std::ctime(&gen_time) << std::endl;


	    asmout << std::endl;
	    asmout << json["_defines"].asString();;
	    asmout << json["_asm"].asString();;

	    std::ofstream hdr(argv[4]);
	    hdr << "// autogenerated from " << infile_name << std::endl
		<< "// " << std::ctime(&gen_time) << std::endl
		<< "#ifndef _SYNTH_H" << std::endl
		<< "#define _SYNTH_H" << std::endl
		<< std::endl
		<< "/* Number of float (32 bit) samples synth can safely generate without" << std::endl
		<< "   internal overflow, i.e. max safe value for 2nd arg of synth()."
		<< std::endl
		<< "   That arg divided by 2 is the number of interleaved stereo frames generated. */"
		<< std::endl
		<< std::endl
		<< "#define SONG_FRAMES ("
		<< data.num_rows << " * " << data.ticklen << " * "
		<< data.order.size() << ")" << std::endl
		<< std::endl
		<< "#ifdef __cplusplus" << std::endl
		<< "extern \"C\" {" << std::endl
		<< "#endif" << std::endl << std::endl
		<< "__attribute__((regparm(2))) void synth(float *outbuf, int samples);"
		<< std::endl << std::endl
		<< "#ifdef __cplusplus" << std::endl
		<< "}" << std::endl
		<< "#endif" << std::endl << std::endl
		<< "#endif /* _SYNTH_H */"
		<< std::endl;

	} catch (TDError e) {
	    std::cerr << "Fatal: " << e.err << std::endl;
	}

   } else { // MODE_RENDER
	std::vector<float> outbuf(44100 * 60);
	struct playback_state playstate;

	synth(outbuf.data(), outbuf.size(), &playstate);

	SF_INFO info;
	info.samplerate = 44100;
	info.channels = 2;
	info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

	SNDFILE *outfile = sf_open(argv[1], SFM_WRITE, &info);
	sf_writef_float(outfile, outbuf.data(), 44100 * 60);
	sf_close(outfile);
	// TODO
    }

    return 0;
}
