#pragma once

#include <vector>
#include <mutex>
#include "pattern.hpp"
#include "module.hpp"
#include "bin_section.hpp"
#include "errors.hpp"

class TrackerData {
protected:
    std::mutex mutex;
public:
    TrackerData();
    TrackerData(unsigned int num_rows, unsigned int num_tracks,
		unsigned int pattern_rows);
    TrackerData(Json::Value &json);

    unsigned int ticklen = 20000; // len of tracker tick in samples
    unsigned int num_rows = 32;
    unsigned int num_tracks = 8;

    static const int max_num_rows = 128;
    static const int max_num_tracks = 8;
    static const int max_num_modules = 64;

    class Trigger {
    public:
	// Trigger this module (index)
	int trig_module_idx = 0;
        // Trigger this parameter of the module
        int trig_param = 0;
    };

    // 2 * 4 per track (4 trigs per instruments, primary & alt instr per track)
    std::vector<Trigger> trigger_points;

    std::vector<uint8_t> order;
    std::vector<Pattern> patterns;
    std::vector<Module> modules;

    // parameters for master high boost filter
    float master_hb_coeff = 1.0;
    float master_hb_mix = 0.0;

    // Optional feature for non-4k builds: per-track bitmasks to skip processing
    // some modules for speedk
    std::vector<uint8_t> module_skip_flags;

    // XXX todo, for const dedup?
    std::map<float, int> float_constants;
    // Constant count for %defines
    int num_constants = 0;
    // Accumulator count
    int num_accums = 0;

    void from_json(Json::Value &json);
    Json::Value as_json();

    // Get binary dumps of data sections for synth
    std::vector<uint8_t> patterns_bin();
    std::vector<uint8_t> order_bin();
    std::vector<uint8_t> modules_bin();
    std::vector<uint8_t> trigger_points_bin();

    // Get all synth data as sections for generating asm source
    std::vector<Section *> bin(bool remove_unused_patterns = false);

    // Append new pattern after others
    void new_pattern();
    // Remove pattern at idx (unless it is the last one)
    bool del_pattern(size_t idx);

    Pattern get_pattern(size_t pattern);
    void set_pattern(size_t pattern, Pattern new_pattern);
    void clear_pattern(size_t pattern);

    Pattern::Track get_track(size_t pattern, size_t track_no);
    void set_track(size_t pattern, size_t track_no, Pattern::Track track);
    void clear_track(size_t pattern, size_t track_no);

    void transpose_pattern(int pattern_idx, int semitones);
    void transpose_pattern_track(int pattern_idx, int track_idx, int semitones);

    void lock();
    void unlock();
};
