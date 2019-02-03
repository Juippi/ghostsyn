#include "tracker_data.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
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

std::vector<Section *> TrackerData::bin() {
    std::cerr << "TrackerData::bin()" << std::endl;

    std::vector<Section *> res;
    int idx;

    res.push_back(new Label("patterns"));
    idx = 0;
    for (auto &pattern : patterns) {
	std::stringstream header;
	header << "pattern " << (idx++);
	res.push_back(new CommentSection(header.str()));
	res.push_back(new BinSection("patterns", pattern.bin(num_rows, num_tracks)));
    }

    BinSection *sect_order = new BinSection("order", "order");;
    for (auto &o : order) {
	sect_order->data.push_back(static_cast<uint8_t>(o));
    }
    res.push_back(sect_order);

    res.push_back(new Label("modules"));
    idx = 0;
    for (auto &module : modules) {
	std::stringstream header;
	header << "module " << (idx++);
	res.push_back(new CommentSection(header.str()));
	auto module_section = module.bin();
	res.insert(res.end(), module_section.begin(), module_section.end());
    }

    BinSection *sect_triggers = new BinSection("trigger_points", "trigger_points");
    for (auto &p : trigger_points) {
	char bin[4];
	uint32_t b_offset = p.trig_module_idx * 20; // TODO: define mod. size as constant

	if (p.trig_type == Trigger::SET_PITCH) {
	    b_offset |= 0x01;
	} else if (p.trig_type == Trigger::SET_ZERO) {
	    b_offset |= 0x02;
	}

	memcpy(bin, &b_offset, 4);
	for (size_t i = 0; i < 4; ++i) {
	    sect_triggers->data.push_back(bin[i]);
	}
    }
    res.push_back(sect_triggers);

    BinSection *skip_flags = new BinSection("module_skip_flags", "module_skip_flags");
    const int max_num_modules = 64; // TODO: to a common header
    for (int tidx : irange(0u, num_tracks)) {
	for (int eidx : irange(0u, modules.size())) {
	    char val = module_skip_flags[tidx * max_num_modules + eidx];
	    skip_flags->data.push_back(val);
	}
    }
    res.push_back(skip_flags);

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
    new_pattern.tracks.resize(num_tracks);
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

void TrackerData::lock() {
    mutex.lock();
}

void TrackerData::unlock() {
    mutex.unlock();
}
