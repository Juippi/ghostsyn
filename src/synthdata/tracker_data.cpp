#include "tracker_data.hpp"
#include "utils.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <sstream>
#include <boost/range/irange.hpp>

using boost::irange;

std::vector<uint8_t> join_sections(std::vector<Section *> sections) {
    std::vector<uint8_t> res;
    for (auto section : sections) {
        std::vector<uint8_t> section_bin = section->bin();
        res.insert(res.begin(), section_bin.begin(), section_bin.end());
    }
    return res;
}

TrackerData::TrackerData() {
}

TrackerData::TrackerData(Json::Value &json) {
    from_json(json);
}

Json::Value TrackerData::as_json() {
    Json::Value json(Json::objectValue);

    json["ticklen"] = ticklen;
    json["num_tracks"] = num_tracks;
    json["num_rows"] = num_rows;

    Json::Value order_json(Json::arrayValue);
    order_json.resize(order.size());
    for (size_t i = 0; i < order.size(); ++i) {
        order_json[static_cast<int>(i)] = order[i];
    }
    json["order"] = order_json;

    Json::Value trigger_points_json(Json::arrayValue);
#if 0
    for (auto &point : trigger_points) {
        Json::Value point_json(Json::arrayValue);
        point_json.append(point.first);
        point_json.append(point.second);
        trigger_points_json.append(point_json);
    }
#endif
    json["trigger_points"] = trigger_points_json;

    Json::Value patterns_json(Json::arrayValue);
    patterns_json.resize(patterns.size());
    for (size_t i = 0; i < patterns.size(); ++i) {
        patterns_json[static_cast<int>(i)] = patterns[i].as_json();
    }
    json["patterns"] = patterns_json;

    Json::Value modules_json(Json::arrayValue);
    modules_json.resize(modules.size());
    for (size_t i = 0; i < modules.size(); ++i) {
        modules_json[static_cast<int>(i)] = modules[i].as_json();
    }
    json["modules"] = modules_json;

    return json;
}

void TrackerData::from_json(Json::Value &json) {
    std::cerr << "load TrackerData" << std::endl;

    trigger_points.clear();
    patterns.clear();
    modules.clear();

    ticklen = json["ticklen"].asInt();
    num_tracks = json["num_tracks"].asInt();
    num_rows = json["num_rows"].asInt();

    Json::Value order_json = json["order"];
    for (auto &p : order_json) {
        std::cerr << "add to order " << p.asInt() << std::endl;
        order.push_back(p.asInt());
    }
#if 0
    for (auto &tp_json : json["trigger_points"]) {
        std::cerr << "add trigger point" << std::endl;
        trigger_points.push_back(std::pair<int, int>(tp_json[0].asInt(), tp_json[1].asInt()));
    }
#endif
    Json::Value patterns_json = json["patterns"];
    for (auto &p : patterns_json) {
        std::cerr << "load pattern" << std::endl;
        Pattern pattern(p);
        pattern.tracks.resize(max_num_tracks);
        pattern.resize(max_num_rows);
        patterns.push_back(pattern);
    }
    Json::Value modules_json = json["modules"];
    for (auto &e : modules_json) {
        std::cerr << "load module" << std::endl;
        modules.push_back(Module(e));
    }
}

std::vector<uint8_t> TrackerData::patterns_bin() {
    std::vector<uint8_t> res;
    for (auto &pattern : patterns) {
        auto pattern_bin = pattern.bin(num_rows, num_tracks);
        res.insert(res.end(), pattern_bin.begin(), pattern_bin.end());
    }
    return res;
}

std::vector<uint8_t> TrackerData::order_bin() {
    return order;
}

std::vector<uint8_t> TrackerData::modules_bin() {
    std::vector<uint8_t> res;
    for (auto &module : modules) {
        auto module_bin = join_sections(module.bin());
        res.insert(res.begin(), module_bin.begin(), module_bin.end());
    }
    return res;
}

std::vector<uint8_t> TrackerData::trigger_points_bin() {
    std::vector<uint8_t> res;
#if 0
    for (auto &p : trigger_points) {
        // dwords
        res.push_back(0);
        res.push_back(0);
        res.push_back(0);
        res.push_back(p.first);
        res.push_back(0);
        res.push_back(0);
        res.push_back(0);
        res.push_back(p.second);
    }
#endif
    return res;
}

std::vector<Section *> TrackerData::bin(bool remove_unused_modules) {
    std::cerr << "TrackerData::bin()" << std::endl;

    std::vector<Section *> res;

    res.push_back(new Label("patterns"));
    // Optimize away patterns not present in play order
    int patterns_written = 0;
    std::map<int, int> order_map;
    for (auto p_idx : irange(0u, patterns.size())) {
        auto &pattern = patterns[p_idx];
        std::stringstream header;
        header << "pattern " << patterns_written;
        if (!remove_unused_modules ||
            std::find(order.begin(), order.end(), p_idx) != order.end()) {
            order_map[p_idx] = patterns_written++;
            res.push_back(new CommentSection(header.str()));
            res.push_back(new BinSection("patterns", pattern.bin(num_rows, num_tracks)));
        }
    }

    BinSection *sect_order = new BinSection("order", "order");;
    for (auto &o : order) {
        sect_order->data.push_back(static_cast<uint8_t>(order_map[o]));
    }
    res.push_back(sect_order);

    /* Algo for creating workbuf contents:

     * iterate trough all parameters in all modules
       - if param is a destination of a connection,
         add pos. for it to accumulator list
       - if param is NOT a destination of a connection,
         add constant value for it to constants list
       - constants go first (nonzeros, then zeros),
         then accumulators
     */

    // determine which params are connection targets
    num_constants = 0;
    int num_nonzero_constants = 0;
    for (auto &module : modules) {
        for (auto &param : module.params) {
            param.is_connection_target = false;
            ++num_constants;
            if (param.value != 0.0f) {
                ++num_nonzero_constants;
            }
        }
    }
    for (auto &module : modules) {
        if (module.out_module >= 0 && module.out_param >= 0) {
            modules[module.out_module].params[module.out_param].is_connection_target = true;
            --num_constants;
        }
    }

    for (auto &p : trigger_points) {
        if (p.trig_module_idx >= 0 && p.trig_param >= 0) {
            modules[p.trig_module_idx].params[p.trig_param].is_trigger_target = true;
        }
    }

    std::vector<float> constants;
    num_accums = 0;
    int num_zero_constants = 0;
    std::map<std::string, int> signal_tgt_map;
    std::map<std::string, int> zeroconst_signal_tgt_map;
    std::map<float, int> deduplicable_constants;

    // TODO:
    //  * static constans could overlap by 1 byte, would probably save some space
    //  * zero-constants that are not trigger or connection targets could be unified

    for (size_t mod_idx : irange(0u, modules.size())) {
        auto &module = modules[mod_idx];
        for (size_t par_idx : irange(0u, module.params.size())) {
            auto &param = module.params[par_idx];
            std::stringstream map_key;
            map_key << mod_idx << "_" << par_idx;

            if (param.is_connection_target) {
                std::cerr << "param " << par_idx <<" of module " << mod_idx
                          << " is signal tgt, reserving accum. " << num_accums << std::endl;
                param.index_in_workbuf = (num_constants + num_accums);
                signal_tgt_map[map_key.str()] = param.index_in_workbuf;
                ++num_accums;
            } else {
                std::cerr << "param " << par_idx <<" of module " << mod_idx
                          << " NOT signal tgt, ";
                auto dedup = deduplicable_constants.find(param.value);
                if (dedup != deduplicable_constants.end() &&
                    param.is_deduplicable()) {
                    std::cerr << "reusing true constant " << param.value << " at "
                              << dedup->second << std::endl;
                    param.index_in_workbuf = dedup->second;
                    signal_tgt_map[map_key.str()] = param.index_in_workbuf;
                } else if (param.value == 0.0f) {
                    std::cerr << "reserving zero const. " << num_zero_constants << std::endl;
                    param.index_in_workbuf = (num_nonzero_constants + num_zero_constants);
                    if (deduplicable_constants.find(param.value) == deduplicable_constants.end() &&
                        param.is_deduplicable()) {
                        deduplicable_constants[param.value] = param.index_in_workbuf;
                    }
                    zeroconst_signal_tgt_map[map_key.str()] = param.index_in_workbuf;
                    ++num_zero_constants;
                } else {
                    std::cerr << "reserving non-zero const. " << constants.size() << std::endl;
                    param.index_in_workbuf = constants.size();
                    if (deduplicable_constants.find(param.value) == deduplicable_constants.end() &&
                        param.is_deduplicable()) {
                        deduplicable_constants[param.value] = param.index_in_workbuf;
                    }
                    signal_tgt_map[map_key.str()] = param.index_in_workbuf;
                    constants.push_back(param.value);
                }
            }
        }
    }

    // set module output indexes in workbuf
    for (auto &module : modules) {
        std::stringstream key;
        key << module.out_module << "_" << module.out_param;
        std::cerr << "handling module out to " << key.str() << std::endl;
        if (module.out_module >= 0 && module.out_param >= 0) {
            // convert to byte offset in workbuf

            size_t outbuf_idx = 0;
            auto i = signal_tgt_map.find(key.str());
            if (i != signal_tgt_map.end()) {
                outbuf_idx = i->second;
            } else {
                i = zeroconst_signal_tgt_map.find(key.str());
                if (i != zeroconst_signal_tgt_map.end()) {
                    outbuf_idx = i->second;
                }
            }

            std::cerr << "output " << key.str() << " goes to " << outbuf_idx << std::endl;
            module.out_workbuf_idx = outbuf_idx;
        } else if (module.out_module == -1) {
            std::cerr << "output " << key.str() << " goes to MASTER at "
                      << num_constants * 4 << std::endl;
            module.out_workbuf_idx = 0; // special reserved value for master out
        }
    }

    std::cerr << "workbuf: got " << constants.size() << " nonzero constants, "
              << num_zero_constants << " zero constants, "
              << num_accums << " accums "
              << std::endl;

    std::vector<float> workbuf(constants.size() + num_zero_constants + num_accums);
    for (size_t i : irange(0u, constants.size())) {
        workbuf[i] = constants[i];
    }

    // Order constraint: modules must be immediately followed by workbuf

    res.push_back(new Label("modules"));
    int idx = 0;
    for (auto &module : modules) {
        std::stringstream header;
        header << "module " << (idx++);
        res.push_back(new CommentSection(header.str()));
        auto module_section = module.bin();
        res.insert(res.end(), module_section.begin(), module_section.end());
    }

    res.push_back(new Label("workbuf"));
    BinSection *sect_workbuf = new BinSection("workbuf", "workbuf");
    for (auto val : workbuf) {
        auto bin_val = float2bin(val);
        sect_workbuf->data.insert(sect_workbuf->data.end(), bin_val.begin(), bin_val.end());
    }
    res.push_back(sect_workbuf);

    BinSection *sect_triggers = new BinSection("trigger_points", "trigger_points");
    for (auto &p : trigger_points) {
        std::cerr << "trigger point: module " << p.trig_module_idx << " param "
                  << p.trig_param << std::endl;
        std::stringstream key;
        key << p.trig_module_idx << "_" << p.trig_param;
        int b_offset = -1;
        auto wp_idx_i = signal_tgt_map.find(key.str());
        if (wp_idx_i != signal_tgt_map.end()) {
            b_offset = wp_idx_i->second;
        } else {
            wp_idx_i = zeroconst_signal_tgt_map.find(key.str());
            if (wp_idx_i != zeroconst_signal_tgt_map.end()) {
                b_offset = wp_idx_i->second;
            }
        }

        if (b_offset >= 0) {
            std::cerr << "found tgt workbuf idx " << wp_idx_i->second << std::endl;
            char bin[sizeof(uint8_t)];
            uint8_t b_offset_u8 = static_cast<uint8_t>(b_offset);
            memcpy(bin, &b_offset_u8, sizeof(uint8_t));
            sect_triggers->data.push_back(bin[0]);
        } else {
            std::cerr << "WARNING: did not find tgt accum offset for trig point "
                      << key.str() << std::endl;
        }
    }
    res.push_back(sect_triggers);

#if 0
    BinSection *skip_flags = new BinSection("module_skip_flags", "module_skip_flags");
    const int max_num_modules = 64; // TODO: to a common header
    for (int tidx : irange(0u, num_tracks)) {
        for (int eidx : irange(0u, modules.size())) {
            char val = module_skip_flags[tidx * max_num_modules + eidx];
            skip_flags->data.push_back(val);
        }
    }
    res.push_back(skip_flags);
#endif

    return res;
}

void TrackerData::new_pattern() {
    patterns.push_back(Pattern(max_num_tracks, max_num_rows));
}

bool TrackerData::del_pattern(size_t pos) {
    if (patterns.size() > 1 && pos < patterns.size()) {
        patterns.erase(patterns.begin() + pos);
        return true;
    } else {
        return false;
    }
}

Pattern TrackerData::get_pattern(size_t pattern) {
    return patterns[pattern];
}

void TrackerData::set_pattern(size_t pattern, Pattern new_pattern) {
    new_pattern.tracks.resize(max_num_tracks);
    new_pattern.resize(max_num_rows);
    patterns[pattern] = new_pattern;
}

void TrackerData::clear_pattern(size_t pattern) {
    for (auto track : irange(0u, num_tracks)) {
        clear_track(pattern, track);
    }
}

Pattern::Track TrackerData::get_track(size_t pattern, size_t track_no) {
    return patterns[pattern].tracks[track_no];
}

void TrackerData::set_track(size_t pattern, size_t track_no, Pattern::Track track) {
    track.resize(max_num_rows);
    patterns[pattern].tracks[track_no] = track;
}

void TrackerData::clear_track(size_t pattern, size_t track_no) {
    patterns[pattern].tracks[track_no].clear();
}

void TrackerData::transpose_pattern(int pattern_idx, int semitones) {
    auto &pattern = patterns[pattern_idx];
    pattern.transpose(semitones);
}

void TrackerData::transpose_pattern_track(int pattern_idx, int track_idx, int semitones) {
    auto &pattern = patterns[pattern_idx];
    pattern.transpose_track(track_idx, semitones);
}

void TrackerData::lock() {
    mutex.lock();
}

void TrackerData::unlock() {
    mutex.unlock();
}
