#include "ghostsyn.hpp"
#include "utils.hpp"
#include <jack/midiport.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <fstream>

int process(jack_nframes_t nframes, void *arg) {
    return static_cast<GhostSyn *>(arg)->process(nframes);
}

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
    
    jack = jack_client_open("ghostsyn", static_cast<jack_options_t>(0), NULL);
    jack_set_process_callback(jack, &::process, static_cast<void *>(this));
    audio_ports[0] = jack_port_register(jack, "left", JACK_DEFAULT_AUDIO_TYPE,
					JackPortIsOutput | JackPortIsTerminal, 0);
    audio_ports[1] = jack_port_register(jack, "right", JACK_DEFAULT_AUDIO_TYPE,
					JackPortIsOutput | JackPortIsTerminal, 0);
    midi_port = jack_port_register(jack, "midi in", JACK_DEFAULT_MIDI_TYPE,
				   JackPortIsInput | JackPortIsTerminal, 0);
    jack_activate(jack);
}

GhostSyn::~GhostSyn() {
    stop();
    jack_client_close(jack);
}

void GhostSyn::load_instrument(int channel, std::string filename) {
    std::ifstream infile(filename);
    instruments[channel].load(infile);
}

void GhostSyn::run() {
    if (!running) {
	jack_activate(jack);
	running = true;
    }
}

void GhostSyn::stop() {
    if (running) {
	jack_deactivate(jack);
	running = false;
    }
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

void GhostSyn::handle_pitch_bend(int channel, int value_1, int value_2) {
    // least significant & most significant 7 bits
    int value = value_1 + 128 * value_2;
    rt_controls[channel][RT_PITCH_BEND].update(0, value);
}

void GhostSyn::handle_midi(jack_midi_event_t &event) {
    if (event.size >= 3) {
	int status = event.buffer[0];
	int channel = status & MIDI_STATUS_MASK_CHANNEL;
	switch (status & MIDI_STATUS_MASK_EVENT) {
	case MIDI_EV_NOTE_ON:
	    handle_note_on(channel, event.buffer[1], event.buffer[2]);
	    break;
	case MIDI_EV_NOTE_OFF:
	    handle_note_off(channel, event.buffer[1], event.buffer[2]);
	    break;
	case MIDI_EV_CC:
	    handle_control_change(channel, event.buffer[1], event.buffer[2]);
	    break;
	case MIDI_EV_PITCH_BEND:
	    handle_pitch_bend(channel, event.buffer[1], event.buffer[2]);
	    break;
	default:
	    break;
	}
    }
}

int GhostSyn::process(jack_nframes_t nframes) {
    jack_default_audio_sample_t *audio_buffers[2];
    void *midi_buffer;
    audio_buffers[0] = static_cast<float *>(jack_port_get_buffer(audio_ports[0], nframes));
    audio_buffers[1] = static_cast<float *>(jack_port_get_buffer(audio_ports[1], nframes));
    midi_buffer = jack_port_get_buffer(midi_port, nframes);

    uint32_t num_events = jack_midi_get_event_count(midi_buffer);
    uint32_t event_idx = 0;
    bool event_pending = false;
    jack_midi_event_t event;
    bool check_midi = true;
    for (jack_nframes_t frame = 0; frame < nframes;) {
	while (check_midi && (event_pending || event_idx < num_events)) {
	    if (!event_pending) {
		jack_midi_event_get(&event, midi_buffer, event_idx);
		event_pending = true;
		++event_idx;
	    }
	    // This assumes events arrive ordered by time
	    if (frame >= event.time) {
		handle_midi(event);
		event_pending = false;
	    } else {
		break;
	    }
	}
	check_midi = false;

	size_t voice_idx = 0;
	for (auto &voice : voices) {
	    if (voice.pressed || voice.sustained) {
		oversample_sum += voice.run(rt_controls[voice.instrument]);
	    }
	    ++voice_idx;
	}

	if (++oversample_ctr == OVERSAMPLE_FACTOR) {
	    double sum = oversample_sum / OVERSAMPLE_FACTOR;
	    double fx_1[2];
	    double fx_2[2];
	    sum = master_dcfilter.run(sum);
	    fx_1[0] = fx_1[1] = sum;
	    master_delay.run(fx_1, fx_2);
	    master_comp.run(fx_2, fx_1);

	    audio_buffers[0][frame] = fx_1[0];
	    audio_buffers[1][frame] = fx_1[1];
	    ++frame;
	    oversample_ctr = 0;
	    oversample_sum = 0.0;
	    check_midi = true;
	    for (auto &voice : voices) {
		if (voice.pressed || voice.sustained) {
		    voice.run_modulation();
		}
	    }
	}
    }
    
    return 0;
}
