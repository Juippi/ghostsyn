#pragma once

#include <vector>
#include <cstdlib>
#include <cstdint>
#include <json/json.h>

class Pattern {
public:
    unsigned int num_tracks;
    unsigned int num_rows;

    Pattern(unsigned int num_tracks, unsigned int num_rows);
    Pattern(Json::Value &json);
    void clear();

    class Cell {
    public:
	Cell();
	Cell(const Json::Value &json);
	void clear();
	Json::Value as_json();
	int note = -1;
	int octave = 0;
	int instrument = 0; // 0 for primary, 1 for alt
	int fx = 0;
	int param = 0;
    };

    class Track {
    public:
	Track(unsigned int num_rows = 0);
	Track(const Json::Value &json);
	void clear();
	void resize(unsigned int num_rows);
	Json::Value as_json();
	std::vector<Cell> cells;
    };

    std::vector<Pattern::Track> tracks;

    void from_json(const Json::Value &json);
    Json::Value as_json();
    // Convert num_rows rows of pattern data to binary representation
    std::vector<uint8_t> bin(unsigned int num_rows);
    void resize(unsigned int num_rows);
};
