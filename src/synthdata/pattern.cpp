#include "pattern.hpp"
#include <algorithm>
#include <iostream>

Pattern::Cell::Cell() {}

Pattern::Cell::Cell(const Json::Value &json) {
    note = json[0].asInt();
    octave = json[1].asInt();
    instrument = json[2].asInt();
}

void Pattern::Cell::clear() {
    note = octave = instrument = -1;
}

Json::Value Pattern::Cell::as_json() {
    Json::Value json(Json::arrayValue);
    json.append(Json::Int(note));
    json.append(Json::Int(octave));
    json.append(Json::Int(instrument));
    return json;
}


Pattern::Track::Track(unsigned int num_rows) {
    resize(num_rows);
}

Pattern::Track::Track(const Json::Value &json) {
    for (auto &c : json) {
	cells.push_back(Pattern::Cell(c));
    }
}

void Pattern::Track::clear() {
    for (auto &cell : cells) {
	cell.clear();
    }
}

Json::Value Pattern::Track::as_json() {
    Json::Value json(Json::arrayValue);
    for (auto &cell : cells) {
	json.append(cell.as_json());
    }
    return json;
}

void Pattern::Track::resize(unsigned int num_rows) {
    cells.resize(num_rows);
}

Pattern::Pattern(unsigned int num_tracks_, unsigned int num_rows_)
    : num_tracks(num_tracks_), num_rows(num_rows_) {
    for (unsigned int i = 0; i < num_tracks; ++i) {
	tracks.push_back(Pattern::Track(num_rows));
    }
}

Pattern::Pattern(Json::Value &json) {
    from_json(json);
}

void Pattern::clear() {
    for (auto &track : tracks) {
	track.clear();
    }
}

void Pattern::from_json(const Json::Value &json) {
    tracks.clear();
    num_tracks = json["num_tracks"].asInt();
    num_rows = json["num_rows"].asInt();
    Json::Value tracks_json = json["tracks"];
    for (size_t i = 0; i < tracks_json.size(); ++i) {
	Json::Value &track_json = tracks_json[static_cast<int>(i)];
	tracks.push_back(Pattern::Track(track_json));
    }
    std::cerr << "load Pattern " << num_tracks << " " << num_rows << std::endl;
}

Json::Value Pattern::as_json() {
    Json::Value pattern_json;

    pattern_json["num_tracks"] = num_tracks;
    pattern_json["num_rows"] = num_rows;

    Json::Value tracks_json(Json::arrayValue);;
    for (size_t i = 0; i < tracks.size(); ++i) {
	tracks_json.append(tracks[i].as_json());
    }
    pattern_json["tracks"] = tracks_json;

    return pattern_json;
}

std::vector<uint8_t> Pattern::bin(unsigned int output_num_rows) {
    std::vector<uint8_t> bin;

    for (size_t r = 0; r < std::min(num_rows, output_num_rows); ++r) {
	for (size_t t = 0; t < num_tracks; ++t) {
	    auto &cell = tracks[t].cells[r];
	    uint8_t val = 0;
	    if (cell.note != -1) {
		val += cell.note % 12;
		val += cell.octave * 12;
		val &= 0x7f;
		val += cell.instrument << 7;
	    }
	    bin.push_back(val);
	}
    }

    bin.resize(output_num_rows * num_tracks);
    return bin;
}

void Pattern::resize(unsigned int num_rows_) {
    num_rows = num_rows_;
    for (auto &t : tracks) {
	t.resize(num_rows);
    }
}
