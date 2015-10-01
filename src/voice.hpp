#ifndef _VOICE_H_
#define _VOICE_H_
#include "instrument.hpp"
#include "controller.hpp"
#include <vector>
#include <cstdint>

class Voice {
    const uint32_t notetable[13] {
	0,
	0x4c5504,
	0x50defc,
	0x55ae0c,
	0x5ac650,
	0x602c23,
	0x65e420,
	0x6bf32b,
	0x725e71,
	0x792b6d,
	0x805ff0,
	0x880221,
	0x901886,
    };
    double oscillator(std::vector<Controller> &rt_controls);
    double filter(double in, std::vector<Controller> &rt_controls);
public:
    int instrument = -1;
    int note = -1;
    int octave = -1;
    std::vector<uint32_t> osc_ctr;

    // Current voice params. Initialized from instrument on note on,
    // modified by envelopes.
    std::vector<double> params;
    std::vector<double> osc_pitches;
    Instrument::OscShape osc_shape;

    // Filter state for this voice
    double flt_p1 = 0.0;
    double flt_p2 = 0.0;

    Voice();
    void set_on(int _instrument, int _note, int _octave, const Instrument &instrument_ref);
    void set_off();
    bool is_on() const { return note >= 0; }
    void run_modulation();

    double run(std::vector<Controller> &rt_controls);
};

#endif // _VOICE_H_
