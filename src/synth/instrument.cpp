#include "instrument.hpp"
#include "globalconfig.hpp"
#include <json/json.h>

Instrument::Instrument()
    : params(NUM_PARAMS) {
}

void Instrument::load(std::istream &in_stream) {
    Json::Reader reader;
    Json::Value parsed;
    if (!reader.parse(in_stream, parsed)) {
	throw std::string("JSON parse failed");
    }
    for (auto &param : name_param_map) {
	params[param.second] = parsed[param.first].asDouble();
    }
    std::string osc_shape = parsed["osc_shape"].asString();
    if (osc_shape == "saw") {
	osc_shape = SAW;
    } else if (osc_shape == "square1") {
	osc_shape = SQUARE1;
    } else if (osc_shape == "square2") {
	osc_shape = SQUARE2;
    } else if (osc_shape == "noise") {
	osc_shape = NOISE;
    }
    for (auto &osc : parsed["oscillators"]) {
	osc_pitches.push_back(osc["pitch"].asDouble());
    }
    osc_pitches.resize(GlobalConfig::max_oscs_per_voice);
}
