#ifndef _GHOSTSYN_H_
#define _GHOSTSYN_H_

#include "compressor.hpp"
#include "dcfilter.hpp"
#include "delay.hpp"
#include "controller.hpp"
#include "instrument.hpp"
#include "voice.hpp"
#include "rt_controls.hpp"
#include <jack/jack.h>
#include <jack/midiport.h>
#include <stdint.h>
#include <vector>
#include <string>

class GhostSyn {
private:
    static const unsigned int OVERSAMPLE_FACTOR = 16;
    static const unsigned int POLYPHONY = 8;

    static const int MIDI_NUM_CHANNELS = 16;
    static const int MIDI_STATUS_MASK_EVENT = 0xf0;
    static const int MIDI_EV_NOTE_OFF = 0x80;
    static const int MIDI_EV_NOTE_ON = 0x90;
    static const int MIDI_EV_POLY_AFTERTOUCH = 0xa0;
    static const int MIDI_EV_CC = 0xb0;
    static const int MIDI_EV_AFTERTOUCH = 0xd0;
    static const int MIDI_EV_PITCH_BEND = 0xe0;
    
    static const int MIDI_STATUS_MASK_CHANNEL = 0x0f;

    std::vector<Instrument> instruments;
    Voice voices[POLYPHONY];

    // Realtime performance controls, one set per MIDI channel. These affect
    // all notes playing on a channel, and do not modify instruments.
    typedef std::vector<Controller> RtControlSet;
    std::vector<RtControlSet> rt_controls;
    std::vector<bool> sustain_controls;

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
    void handle_control_change(int channel, int control, int value);
    void handle_pitch_bend(int channel, int value_1, int value_2);
    void handle_midi(jack_midi_event_t &event);

public:
    GhostSyn();
    ~GhostSyn();
    void load_instrument(int channel, std::string filename);
    void run();
    void stop();
    int process(jack_nframes_t nframes);
};

#endif // _GHOSTSYN_H_
