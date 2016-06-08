#include "ghostsyn.hpp"
#include "utils.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <fstream>

GhostSyn::GhostSyn()
    : instruments(MIDI_NUM_CHANNELS), sustain_controls(MIDI_NUM_CHANNELS, false) {

    rt_controls.resize(MIDI_NUM_CHANNELS);
    for (auto &ctrl_set : rt_controls) {
	ctrl_set.resize(RT_NUM_TYPES);
	ctrl_set[RT_FILTER_CUTOFF] = Controller(Controller::TYPE_LINEAR,
						Controller::RANGE_8BIT,
						0.1, 1.0,
						63, 0.55,
						63);
	ctrl_set[RT_PITCH_BEND] = Controller(Controller::TYPE_LINEAR,
					     Controller::RANGE_14BIT,
					     0.94387, 1.05946,
					     8192, 1.0,
					     8192);
    }
}

GhostSyn::~GhostSyn() {
}

void GhostSyn::load_instrument(int channel, std::string filename) {
    std::ifstream infile(filename);
    instruments[channel].load(infile);
}

void GhostSyn::handle_note_on(int channel, int midi_note, int velocity) {
    unused(velocity);
    // TODO: note stealing
    int idx = 0;
    Instrument &instr = instruments[channel];
    int note = midi_note % 12;
    int octave = midi_note / 12;
    for (auto &voice : voices) {
	// Don't trigger a second copy of a sustained note
	if (voice.sustained && voice.note == note && voice.octave == octave) {
	    return;
	}
    }
    for (auto &voice : voices) {
	if (!voice.pressed && !voice.sustained) {
	    voice.set_on(channel, midi_note % 12, midi_note / 12, instr);
	    if (sustain_controls[channel]) {
		voice.sustained = true;
	    }
	    break;
	}
	++idx;
    }
}

void GhostSyn::handle_note_off(int channel, int midi_note, int velocity) {
    unused(velocity);
    int note = midi_note % 12;
    int octave = midi_note / 12;
    for (auto &voice : voices) {
	if (voice.pressed && voice.note == note &&
	    voice.octave == octave && voice.instrument == channel) {
	    voice.set_off();
	}
    }
}

void GhostSyn::handle_control_change(int channel, int control, int value) {
    switch (control) {
    case 0x01:
	rt_controls[channel][RT_FILTER_CUTOFF].update(0, value); // TODO: timestamp in
	break;
    case 0x40:
	if (value >= 64) {
	    sustain_controls[channel] = true;
	    for (auto &voice : voices) {
		if (voice.pressed && voice.instrument == channel) {
		    voice.sustained = true;
		}
	    }
	} else {
	    sustain_controls[channel] = false;
	    for (auto &voice : voices) {
		if (voice.instrument == channel) {
		    voice.sustained = false;
		}
	    }
	}
	break;
    }
}

void GhostSyn::render(float *out_buf[2], uint32_t offset, uint32_t sample_count) {
    for (uint32_t frame = 0; frame < sample_count;) {

	for (auto &voice : voices) {
	    if (voice.pressed || voice.sustained) {
		oversample_sum += voice.run(rt_controls[voice.instrument]);
	    }
	}

	if (++oversample_ctr == OVERSAMPLE_FACTOR) {
	    double sum = oversample_sum / OVERSAMPLE_FACTOR;
	    double fx_1[2];
	    double fx_2[2];
	    sum = master_dcfilter.run(sum);
	    fx_1[0] = fx_1[1] = sum;
	    master_delay.run(fx_1, fx_2);
	    master_comp.run(fx_2, fx_1);
	    
	    out_buf[0][offset + frame] = fx_1[0];
	    out_buf[1][offset + frame] = fx_1[1];
	    ++frame;
	    oversample_ctr = 0;
	    oversample_sum = 0.0;
	    for (auto &voice : voices) {
		if (voice.pressed || voice.sustained) {
		    voice.run_modulation();
		}
	    }
	}
    }
}
