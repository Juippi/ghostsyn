#include "instruments.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cstdio>

int Instruments::map_out(size_t instrument_off, int out_idx, int tgt_param_idx) {
    int val;
    if (out_idx < 0) {
	// master out, will be handled in bin()
	val = -1;
	std::cerr << "master out offset " << val << std::endl;
    } else {
	// regular connections, input is 3rd module param
	// these are relative references to current instrument: next one is 1 etc
	val = static_cast<int>((sizeof(float) * Instruments::max_params * out_idx) +
			       (tgt_param_idx * sizeof(float)));
	std::cerr << "elem out idx " << out_idx << " maps to offset " << val << std::endl;
    }
    return val;
}

Instruments::Module::Param::Param(const int &val) :
    type(Instruments::Module::Param::TYPE_INT32), int_value(val) {}

Instruments::Module::Param::Param(const double &val) :
    type(Instruments::Module::Param::TYPE_FLOAT), float_value(val) {}

void Instruments::load(const std::string filename) {
    modules.clear();
    env_triggers.clear();
    osc_pitches.clear();
    
    std::ifstream infile(filename.c_str());
    Json::Reader reader;
    Json::Value parsed;
    reader.parse(infile, parsed);

    Json::Value instruments = parsed["instruments"];

    // count modules to determine master out offset
    num_modules = 0;
    for (auto &instrument : instruments) {
	num_modules += instrument["modules"].size();
    }
    std::cerr << "total mod count " << num_modules << std::endl;

    for (auto &instrument : instruments) {
	// byte offset of instr start, element output targets are relative to this
	size_t instrument_off = modules.size() * sizeof(float) * Instruments::max_params;
	bool has_env_trigger = false;
	bool has_osc_trigger = false;

	size_t module_in_instr_idx = 0;
	for (auto &module_json : instrument["modules"]) {
	    Instruments::Module module;
	    
	    if (module_json["type"] == "oscillator") {
		int type = Module::TYPE_OSC;
		if (module_json.get("stereo", false).asBool()) {
		    type |= Module::FLAG_STEREO;
		}
		if (module_json["op"] == "add") {
		    type |= 0x100;
		}
		if (module_json["op"] == "mult") {
		    type |= 0x200;
		}
		module.params.push_back(Module::Param(type));

		
		module.params.push_back(Module::Param(map_out(instrument_off,
							      module_json["out"].asInt(),
							      module_json.get("out_param", 2).asInt())));
		module.params.push_back(Module::Param(module_json["gain"].asDouble()));
		module.params.push_back(Module::Param(0.0f)); // freq mod
		module.params.push_back(Module::Param(module_json["add"].asDouble()));
		module.params.push_back(Module::Param(module_json["detune2"].asDouble()));

		if (module_json.get("tracker_ctl", false).asBool() && !has_osc_trigger) {
		    std::cerr << "osc trig point in instr module " << module_in_instr_idx << std::endl;
		    has_osc_trigger = true;
		    // tracker writers pitches here (relatite off. to instr start)
		    osc_pitches.push_back(module_in_instr_idx * sizeof(float) * Instruments::max_params + 8);
		}

	    } else if (module_json["type"] == "filter") {
		module.params.push_back(Module::TYPE_FILTER); // todo: operator?
		module.params.push_back(Module::Param(map_out(instrument_off, module_json["out"].asInt(), module_json.get("out_param", 2).asInt())));
		module.params.push_back(0); // input
		module.params.push_back(Module::Param(module_json["cutoff"].asDouble()));
		module.params.push_back(Module::Param(module_json["feedback"].asDouble()));

	    } else if (module_json["type"] == "envelope") {
		module.params.push_back(Module::TYPE_ENV); // todo: operator?
		module.params.push_back(Module::Param(map_out(instrument_off, module_json["out"].asInt(), module_json.get("out_param", 2).asInt())));
		module.params.push_back(Module::Param(module_json["attack"].asDouble()));
		module.params.push_back(Module::Param(module_json["switch"].asDouble()));
		module.params.push_back(Module::Param(module_json["release"].asDouble()));

		if (module_json.get("tracker_ctl", false).asBool() && !has_env_trigger) {
		    std::cerr << "env trig point in instr module " << module_in_instr_idx << std::endl;
		    has_env_trigger = true;
		    // tracker triggers this envelope (relative off. to instr start)
		    env_triggers.push_back(module_in_instr_idx * sizeof(float) * Instruments::max_params + 8);
		}

	    } else if (module_json["type"] == "compressor") {
		module.params.push_back(Module::TYPE_COMPRESSOR);
		module.params.push_back(Module::Param(map_out(instrument_off, module_json["out"].asInt(), module_json.get("out_param", 2).asInt())));
		module.params.push_back(0); // input
		module.params.push_back(Module::Param(module_json["threshold"].asDouble()));
		module.params.push_back(Module::Param(module_json["attack"].asDouble()));
		module.params.push_back(Module::Param(module_json["release"].asDouble()));

	    } else if (module_json["type"] == "delay") {
		module.params.push_back(Module::TYPE_DELAY);
		module.params.push_back(Module::Param(map_out(instrument_off, module_json["out"].asInt(), module_json.get("out_param", 2).asInt())));
		module.params.push_back(0) ; // input
		module.params.push_back(Module::Param(module_json["time"].asInt()));
		module.params.push_back(Module::Param(module_json["feedback"].asDouble()));
	    }

	    std::cerr << "load module" << std::endl;
	    modules.push_back(module);

	    ++module_in_instr_idx;
	}

	if (!has_env_trigger || !has_osc_trigger) {
	    std::cerr << "WARNING: missing trigger connection points " << has_env_trigger
		      << " " << has_osc_trigger << std::endl;
	}
    }
}

void binout(const int &val, unsigned char *dst) {
    memcpy(dst, &val, sizeof(val));
}

void binout(const float &val, unsigned char *dst) {
    memcpy(dst, &val, sizeof(val));
    memset(dst, 0, 1); // truncate float constant
     // memset(dst + 1, 0, 1); // truncate float constant
}

std::vector<uint8_t> Instruments::bin() {

    size_t data_len = modules.size() * sizeof(float) * Instruments::max_params;
    std::cerr << data_len << " bytes for modules" << std::endl;
    size_t off = 0;

    std::vector<uint8_t> res(data_len, 0);
   
    unsigned int mod_idx = 0;
    for (const auto &module : modules) {
	for (auto &param : module.params) {
	    if (param.type == Instruments::Module::Param::TYPE_INT32) {
		binout(param.int_value, res.data() + off);
	    } else { // float
		binout(param.float_value, res.data() + off);
	    }
	    off += sizeof(float);
	}
	++mod_idx;
	off = mod_idx * sizeof(float) * Instruments::max_params;
    }
    std::cerr << "total bin size: " << res.size() << std::endl;

    print_words(res.data(), res.size(), 24);

    return res;
}
