#include "ghostsyn.hpp"
#include <jack/midiport.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <fstream>
#include <json/json.h>

int process(jack_nframes_t nframes, void *arg) {
    return static_cast<GhostSyn *>(arg)->process(nframes);
}

GhostSyn::Voice::Voice()
    : osc_ctr(MAX_INSTRUMENT_OSCS, 1 << 30) {
}

GhostSyn::Instrument::Instrument()
    : pitches(MAX_INSTRUMENT_OSCS, 0) {
    
}

GhostSyn::GhostSyn()
    : instruments(MIDI_NUM_CHANNELS) {

    for (auto &channel_controllers : controllers) {
	channel_controllers[CTRL_FILTER_CUTOFF] = Controller(1.0, 0.1, 1.0,
							     Controller::TYPE_LINEAR,
							     Controller::RANGE_8BIT);
	channel_controllers[CTRL_PITCH_BEND] = Controller(1.0, 0.94387, 1.05946, 1.0,
							  Controller::TYPE_LINEAR,
							  Controller::RANGE_14BIT);
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
    Instrument &instr = instruments[channel];
    Json::Reader reader;
    Json::Value parsed;
    if (!reader.parse(infile, parsed)) {
	throw std::string("JSON parse failed");
    }
    instr.volume = parsed["volume"].asDouble();
    instr.filter_co = parsed["filter_cutoff"].asDouble();
    instr.filter_decay = parsed["filter_decay"].asDouble();
    instr.filter_fb = parsed["filter_feedback"].asDouble();
    instr.amp_decay = parsed["amp_decay"].asDouble();
    instr.pitch_decay = parsed["pitch_decay"].asDouble();
    instr.pitch_min = parsed["pitch_min"].asDouble();
    std::string osc_shape = parsed["osc_shape"].asString();
    if (osc_shape == "saw") {
	instr.shape = Instrument::SAW;
    } else if (osc_shape == "square1") {
	instr.shape = Instrument::SQUARE1;
    } else if (osc_shape == "square2") {
	instr.shape = Instrument::SQUARE2;
    } else if (osc_shape == "noise") {
	instr.shape = Instrument::NOISE;
    }
    size_t osc_idx = 0;
    for (auto &osc : parsed["oscillators"]) {
	if (osc_idx > instr.pitches.size()) {
	    break;
	}
	instr.pitches[osc_idx] = osc["pitch"].asDouble();
	++osc_idx;
    }
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

double GhostSyn::filter(double in, Voice &voice) {
    GhostSyn::Instrument &instr = instruments[voice.instrument];
    double cutoff = voice.flt_co * controllers[voice.instrument][CTRL_FILTER_CUTOFF].get_value();
    double fb_amt = instr.filter_fb * (cutoff * 3.296875f - 0.00497436523438f);
    double feedback = fb_amt * (voice.flt_p1 - voice.flt_p2);
    voice.flt_p1 = in * cutoff +
	voice.flt_p1 * (1 - cutoff) +
	feedback + std::numeric_limits<double>::min();
    voice.flt_p2 = voice.flt_p1 * cutoff * 2 +
	voice.flt_p2 * (1 - cutoff * 2) + std::numeric_limits<double>::min();
    
    return voice.flt_p2;
}

double GhostSyn::oscillator(Voice &voice) {
    GhostSyn::Instrument &instr = instruments[voice.instrument];
    double osc_out = 0;
    for (size_t i = 0; i < instr.pitches.size(); ++i) {
	if (instr.shape == Instrument::NOISE) {
	    osc_out += (rand() % 65536 - 32768) / 32768.0;
	} else {
	    if (instr.pitches[i] != 0) {
		voice.osc_ctr[i] += (double(GhostSyn::notetable[voice.note + 1]) *
				     (1 << voice.octave) / 128) *
		    instr.pitches[i] * voice.pitch * controllers[voice.instrument][CTRL_PITCH_BEND].get_value();
		int32_t osc_int = (voice.osc_ctr[i] >> 1) - (1 << 30);
		if (instr.shape == Instrument::SQUARE1) {
		    osc_int &= 0x80000000;
		} else if (instr.shape == Instrument::SQUARE2) {
		    osc_int &= 0xc0000000;
		}
		osc_out += osc_int / (double(1LL << 32));
	    }
	}
    }
    return osc_out * voice.vol;
}

void GhostSyn::run_modulation(Voice &voice) {
    Instrument &instr = instruments[voice.instrument];
    if (voice.flt_co < ENVELOPE_MIN) {
	voice.flt_co = 0;
    } else {
	voice.flt_co *= instr.filter_decay;
    }
    if (voice.vol < ENVELOPE_MIN) {
	voice.vol = 0;
    } else {
	voice.vol *= instr.amp_decay;
    }
    voice.pitch = instr.pitch_decay * (voice.pitch + std::numeric_limits<double>::min());
    if (voice.pitch < instr.pitch_min) {
	voice.pitch = instr.pitch_min;
    }
}

void GhostSyn::handle_note_on(int channel, int midi_note, int velocity) {
    // TODO: note stealing
    int idx = 0;
    Instrument &instr = instruments[channel];
    for (auto &voice : voices) {
	if (voice.note == -1) {
	    voice.instrument = channel;
	    voice.note = midi_note % 12;
	    voice.octave = midi_note / 12;
	    voice.flt_co = instr.filter_co;
	    voice.vol = instr.volume;
	    voice.pitch = 1.0;
	    break;
	}
	++idx;
    }
}

void GhostSyn::handle_note_off(int channel, int midi_note, int velocity) {
    int note = midi_note % 12;
    int octave = midi_note / 12;
    int idx = 0;
    for (auto &voice : voices) {
	if (voice.note == note && voice.octave == octave &&
	    voice.instrument == channel) {
	    voice.note = -1;
	}
	++idx;
    }
}

void GhostSyn::handle_control_change(int channel, int control, int value) {
    switch (control) {
    case 0x01:
	controllers[channel][CTRL_FILTER_CUTOFF].update(0, value); // TODO: timestamp in
	break;
    }
}

void GhostSyn::handle_pitch_bend(int channel, int value_1, int value_2) {
    // least significant & most significant 7 bits
    int value = value_1 + 128 * value_2;
    controllers[channel][CTRL_PITCH_BEND].update(0, value);
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

	for (auto &voice : voices) {
	    double osc_out = oscillator(voice);
	    oversample_sum += filter(osc_out, voice);
	    run_modulation(voice);
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
	}
    }
    
    return 0;
}
