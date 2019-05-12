#include <cstdint>
#include <sndfile.h>
#include <vector>
#include <iostream>
#include "synth_api.h"

#define SAMPLES 44100 * 2

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "testprog <outfile>" << std::endl;
        return 1;
    }
    
    std::vector<float> outbuf(SAMPLES);
    SF_INFO info;

    synth(outbuf.data(), outbuf.size(), 0);

    // for (size_t i = 0; i < 12; ++i) {
    //     std::cout << workbuf[i] << std::endl;
    // }
    // std::cout << "---" << std::endl;

    info.samplerate = 44100;
    info.channels = 2;
    info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE *outfile = sf_open(argv[1], SFM_WRITE, &info);
    sf_writef_float(outfile, outbuf.data(), outbuf.size() / info.channels);
    sf_close(outfile);
    
    return 0;
}
