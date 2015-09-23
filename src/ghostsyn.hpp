#ifndef _GHOSTSYN_H_
#define _GHOSTSYN_H_

#include "compressor.hpp"
#include "dcfilter.hpp"
#include "delay.hpp"
#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdint.h>
#include <vector>
#include <string>

class GhostSyn {
private:
    static const unsigned int OVERSAMPLE_FACTOR = 16;
    static const unsigned int POLYPHONY = 8;
    static const unsigned int MAX_INSTRUMENT_OSCS = 6;

    static const int MIDI_NUM_CHANNELS = 16;
    static const int MIDI_STATUS_MASK_EVENT = 0xf0;
    static const int MIDI_EV_NOTE_OFF = 0x80;
    static const int MIDI_EV_NOTE_ON = 0x90;

    static const int MIDI_STATUS_MASK_CHANNEL = 0x0f;

    uint32_t notetable[13] {
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
    
    class Instrument {
    public:
	double filter_co = 0.5;
	double filter_decay = 0.0;
	double filter_fb = 0.0;
	double volume = 0.0;
	double amp_decay = 0.0;
	double pitch_decay = 0.0;
	double pitch_min = 0.01;
	std::vector<double> pitches;
	enum {SAW, TRIANGLE, NOISE, SQUARE1, SQUARE2} shape = SAW;

	Instrument();
    };
    
    class Voice {
    public:
	int note = -1;
	int instrument = 0;
	int octave = 0;
	std::vector<uint32_t> osc_ctr;

	double vol = 0.0;   // current volume
	double pitch = 1.0; // current pitch mod
	// filter state
	double flt_co = 0; // current cutoff
	double flt_p1 = 0;
	double flt_p2 = 0;

	Voice();
    };

    std::vector<Instrument> instruments;
    Voice voices[POLYPHONY];

    bool running = false;
    jack_client_t *jack;
    jack_port_t *audio_ports[2];
    jack_port_t *midi_port;

    unsigned int oversample_ctr = 0;
    double oversample_sum = 0.0;

    Delay master_delay;
    Compressor master_comp;
    DCFilter master_dcfilter;

    void handle_note_on(int channel, int midi_note, int velocity);
    void handle_note_off(int channel, int midi_note, int velocity);
    void handle_midi(jack_midi_event_t &event);

    double filter(double in, Voice &voice);
    double oscillator(Voice &voice);
    void run_modulation(Voice &voice);

public:
    GhostSyn();
    ~GhostSyn();
    void load_instrument(int channel, std::string filename);
    void run();
    void stop();
    int process(jack_nframes_t nframes);
};

#endif // _GHOSTSYN_H_
