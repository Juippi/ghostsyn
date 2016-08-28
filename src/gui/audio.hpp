#pragma once

#include "tracker_data.hpp"
#include <SDL_audio.h>
#include <atomic>

class PlaystateCBInterface {
public:
    virtual void report_playstate(unsigned int current_pattern,
				  unsigned int current_row) = 0;
};

class Audio {
protected:
    SDL_AudioDeviceID device;
    TrackerData &data;
    PlaystateCBInterface &playstate_cb;

    enum {
	STATE_STOPPED,
	STATE_PLAYING_PATTERN,
	STATE_PLAYING_SONG
    } state = STATE_STOPPED;

    std::atomic_bool synth_running = false;
    std::mutex synth_mutex;

   // Number of pattern to play in STATE_PLAYING_PATTERN
    size_t play_single_pattern_no = 0;

public:
    Audio(TrackerData &data, PlaystateCBInterface &playstate_cb);
    ~Audio();

    // SDL audio callback
    void callback(Uint8 *stream, int len);
    // Update internal synth data from tracker data
    void update_synth(int order_offset);

    void play_pattern(int pattern, int from_row);
    void play_song(int from_order);
    void stop();
};
