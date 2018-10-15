#include "audio.hpp"
#include "utils.hpp"
#include "synth_api.h"
#include <string>
#include <iostream>
#include <algorithm>

std::vector<uint8_t> find_section(const std::vector<Section *> sections, const std::string &type) {
    std::vector<uint8_t> data;
    for (auto *section: sections) {
	BinSection *bs = dynamic_cast<BinSection *>(section);
	if (bs && bs->type == type) {
	    std::cerr << "insert " << type << " " << bs->data.size() << std::endl;
	    data.insert(data.end(), bs->data.begin(), bs->data.end());
	}
    }
    return data;
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    Audio *a = static_cast<Audio *>(userdata);
    a->callback(stream, len);
}

Audio::Audio(TrackerData &data_, PlaystateCBInterface &playstate_cb_)
    : data(data_), playstate_cb(playstate_cb_) {
    std::cerr << "Audio" << std::endl;

    SDL_AudioSpec spec;
    SDL_AudioSpec got;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = 1024;
    spec.callback = audio_callback;
    spec.userdata = this;
    device = SDL_OpenAudioDevice(NULL, 0, &spec, &got, 0);
    if (device == 0) {
	std::cerr << SDL_GetError() << std::endl;
    }
    SDL_PauseAudioDevice(device, 0);
}

Audio::~Audio() {
    SDL_CloseAudioDevice(device);
}

void Audio::callback(Uint8 *stream, int len) {
    memset(stream, 0, len);
    size_t frames = len / (sizeof(float) * 2);
    float *float_stream = reinterpret_cast<float *>(stream);
    struct playback_state playstate;

    if (synth_running) {
	// synth requires sample count, not stereo frame count
#ifdef DEBUG
	std::cerr << "call synth, samples = " << frames * 2 << std::endl;
#endif
	{
	    std::unique_lock<std::mutex> lock(synth_mutex);
	    synth(float_stream, frames * 2, &playstate);
	}
	playstate_cb.report_playstate(playstate.current_pattern,
				      playstate.current_row);
#ifdef DEBUG
	for (size_t i = 0; i < std::min(frames * 2, 16u); ++i) {
	    std::cerr << float_stream[i] << " ";
	}
	std::cerr << std::endl;
#endif
    }
}

void Audio::update_synth(int order_offset) {
    data.lock();
    std::vector<Section *> bin = data.bin();
    int module_count = static_cast<int>(data.modules.size());
    int ticklen = static_cast<int>(data.ticklen);
    data.unlock();

    auto section_patterns = find_section(bin, "patterns");
    auto section_order = find_section(bin, "order");
    auto section_modules = find_section(bin, "modules");
    auto section_triggers = find_section(bin, "trigger_points");

    std::cerr << "update synth! " << std::endl;

    std::cerr << "patterns (" << section_patterns.size() << " bytes):" << std::endl;
    print_words(section_patterns.data(), section_patterns.size(), 16);

    if (state == STATE_PLAYING_PATTERN) {
	// instead of song, just loop a single pattern
	section_order.resize(32);
	for (auto &v : section_order) {
	    v = play_single_pattern_no;
	}
    }

    // playing song from the middle is accomplished by rotating the order
    if (order_offset > 0 && order_offset < static_cast<int>(section_order.size())) {
	std::rotate(section_order.begin(), section_order.begin() + order_offset, section_order.end());
    }

    std::cerr << "order (" << section_order.size() << " bytes):" << std::endl;
    print_words(section_order.data(), section_order.size(), 16);

    std::cerr << "modules (" << data.modules.size() << ", "
	      << section_modules.size() << " bytes):" << std::endl;
    print_words(section_modules.data(), section_modules.size(), 24);

    std::cerr << "triggers (" << section_triggers.size() << " bytes):" << std::endl;
    print_words(section_triggers.data(), section_triggers.size(), 16);

    std::cerr << "skip flags:" << std::endl;
    print_words(data.module_skip_flags.data(), data.module_skip_flags.size(), 16);

    if (section_patterns.size() > 0 && section_order.size() > 0 &&
	section_modules.size() > 0 && section_triggers.size() > 0) {
	std::unique_lock<std::mutex> lock(synth_mutex);
	synth_update(section_patterns.data(), section_patterns.size(),
		     section_order.data(), section_order.size(),
		     section_modules.data(), section_modules.size(),
		     section_triggers.data(), section_triggers.size(),
		     module_count, ticklen,
		     data.module_skip_flags.data(), data.module_skip_flags.size());
    } else {
	std::cerr << "some data section(s) empty, can't configure synth!" << std::endl;
    }
}

void Audio::play_pattern(int pattern, int from_row) {
    state = STATE_PLAYING_PATTERN;
    play_single_pattern_no = pattern;
    update_synth(0);
    synth_running = true;
}

void Audio::play_song(int from_order) {
    state = STATE_PLAYING_SONG;
    play_single_pattern_no = 0;
    update_synth(from_order);
    synth_running = true;
}

void Audio::stop() {
    synth_running = false;
    state = STATE_STOPPED;
}
