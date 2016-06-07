#include "ghostsyn.hpp"

#include <unistd.h>
#include <signal.h>
#include <iostream>

bool quit = false;

int main(int argc, char *argv[]) {
    GhostSyn synth;
    for (int i = 1; i < argc; ++i) {
	std::cerr << "Load instrument for midi channel " << i << ": " << argv[i] << std::endl;
	try {
	    synth.load_instrument(i - 1, argv[i]);
	} catch (std::string error) {
	    std::cerr << "Warning: failed to load " << argv[i] << " : "
		      << error << std::endl;
	}
    }
    synth.run();
    while (!quit) {
	sleep(1);
    }
    synth.stop();
    return 0;
}
