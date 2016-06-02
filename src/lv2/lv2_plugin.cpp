#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include <stdlib.h>

#include "../ghostsyn.hpp"

/**
 * LV2 plugin wrapper for Ghostsyn
 *
 * Based on the lv2 plugin examples at http://lv2plug.in/git/cgit.cgi/lv2.git/
 */

#define GHOSTSYN_URI "http://example.com/plugins/lv2_ghotsyn"

typedef enum {
    MIDI_IN = 0,
    OUT_LEFT = 1,
    OUT_RIGHT = 2
} PortIndex;

typedef struct {
    GhostSyn *synth;
    
    const LV2_Atom_Sequence *midi_in;
    const float *audio_out[2];

    LV2_URID_Map* map;
    struct {
	LV2_URID midi_MidiEvent;
    } uris;
} GhostsynStruct;

extern "C" {
    
static LV2_Handle instantiate(const LV2_Descriptor* /* descriptor */,
			      double /* rate */,
			      const char*  /* bundle_path */,
			      const LV2_Feature* const* features) {
    LV2_URID_Map* map = NULL;
    for (int i = 0; features[i]; ++i) {
	if (!strcmp(features[i]->URI, LV2_URID__map)) {
	    map = (LV2_URID_Map*)features[i]->data;
	    break;
	}
    }

    GhostsynStruct *self = (GhostsynStruct *)calloc(1, sizeof(GhostSyn));
    self->synth = new GhostSyn();
    self->map = map;
    self->uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);

    return (LV2_Handle)self;
}

static void connect_port(LV2_Handle instance,
			 uint32_t port,
			 void *data) {
    GhostsynStruct *self = (GhostsynStruct *)instance;

    switch ((PortIndex)port) {
    case MIDI_IN:
	self->midi_in = (const LV2_Atom_Sequence *)data;
	break;
    case OUT_LEFT:
	self->audio_out[0] = (const float *)data;
	break;
    case OUT_RIGHT:
	self->audio_out[1] = (const float *)data;
	break;
    }
}

static void activate(LV2_Handle instance) {
}

    static void run(LV2_Handle instance, uint32_t sample_count) {
    GhostsynStruct *self = (GhostsynStruct *)instance;
    uint32_t offset = 0;

    LV2_ATOM_SEQUENCE_FOREACH(self->midi_in, ev) {
	if (ev->body.type == self->uris.midi_MidiEvent) {
	    const uint8_t* const msg = (const uint8_t*)(ev + 1);
	    switch (lv2_midi_message_type(msg)) {
	    case LV2_MIDI_MSG_NOTE_ON:
		// TODO
		break;
	    case LV2_MIDI_MSG_NOTE_OFF:
		// TODO
		break;
	    default:
		break;
	    }
	}

	// TODO: write N frames to both output ports
	// write_output(self, offset, ev->time.frames - offset);
	offset = (uint32_t)ev->time.frames;
    }
    // TODO: write the remainder, if any
    // write_output(self, offset, sample_count - offset);
}

static void deactivate(LV2_Handle instance) {
}

static void cleanup(LV2_Handle instance) {
    GhostsynStruct *self = (GhostsynStruct *)instance;
    delete self->synth;
    free(instance);
}

static const void* extension_data(const char *uri) {
    return NULL;
}

static const LV2_Descriptor descriptor = {
    GHOSTSYN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

}; // extern "C"
