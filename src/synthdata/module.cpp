#include "module.hpp"
#include <cstring>
#include <iostream>

const std::map<Module::ModuleType, std::string> Module::type_names = {
    {TYPE_OSC, "oscillator"},
    {TYPE_FILTER, "filter"},
    {TYPE_ENV, "envelope"},
    {TYPE_COMP, "compressor"},
    {TYPE_REVERB, "reverb"},
    {TYPE_CHORUS, "chorus"}
};

const std::map<std::string, Module::ModuleType> Module::name_types = {
	{"oscillator", TYPE_OSC},
	{"filter", TYPE_FILTER},
	{"envelope", TYPE_ENV},
	{"compressor", TYPE_COMP},
	{"reverb", TYPE_REVERB},
	{"chorus", TYPE_CHORUS}
};

// Convert floating point value to byte array & truncate to N bits
std::vector<uint8_t> float2bin(const float value, unsigned int bits = 24) {
    std::vector<uint8_t> bytes(sizeof(float));
    memcpy(bytes.data(), &value, sizeof(float));
    for (unsigned int i = 0; i < (32 - bits); ++i) {
	bytes[i / 8] &= ~(0x80 >> (i % 8));
    }
    return bytes;
}

std::string Module::type2str(int _type) {
    switch (_type) {
    case TYPE_OSC:
	return "TYPE_OSC";
    case TYPE_FILTER:
	return "TYPE_FILTER";
    case TYPE_ENV:
	return "TYPE_ENV";
    case TYPE_COMP:
	return "TYPE_COMP";
    case TYPE_REVERB:
	return "TYPE_REVERB";
    case TYPE_CHORUS:
	return "TYPE_CHORUS";
    default:
	return "unknown";
    }
}

std::vector<uint8_t> Module::Param::bin() {
    switch (type) {
    case TYPE_INT32:
    {
	int32_t val = static_cast<int32_t>(int_value);
	std::vector<uint8_t> res(sizeof(val));
	memcpy(res.data(), &val, sizeof(val));
	return res;
	break;
    }
    case TYPE_FLOAT:
	return float2bin(float_value, float_trunc_bits);
    default:
	return std::vector<uint8_t>(4);
    }
}

Module::Param::Param() {
    type = TYPE_INT32;
    int_value = 0;
}

Module::Param::Param(const std::string &name_, float value, int trunc_bits)
    : name(name_), float_trunc_bits(trunc_bits) {
    type = TYPE_FLOAT;
    float_value = value;
}

Module::Param::Param(const std::string &name_, const int value) : name(name_) {
    type = TYPE_INT32;
    int_value = value;
}

Json::Value Module::Param::as_json() const {
    if (type == TYPE_INT32) {
	return Json::Value(int_value);
    } else {
	return Json::Value(float_value);
    }
}

Module::Module() {}

Module::Module(Json::Value &json) {
    from_json(json);
}

Module::Module(ModuleType type_)
    : type(type_) {
    switch (type) {
    case TYPE_OSC:
	params.push_back(Param("gain", 0.1f, 16));
	params.push_back(Param("freq_mod", 0.0f));
	params.push_back(Param("add", 0.0f));
	params.push_back(Param("detune", 1.0f));
	break;
    case TYPE_FILTER:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("cutoff", 0.5f));
	params.push_back(Param("feedback", 0.0f));
	break;
    case TYPE_ENV:
	params.push_back(Param("attack", 0.1f, 16));
	params.push_back(Param("switch", 0.5f));
	params.push_back(Param("release", 0.999f));
	// needs to be in decay state initially to avoid spurious trigger at start
	params.push_back(Param("stage", 1));
	break;
    case TYPE_COMP:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("threshold", 0.5f));
	params.push_back(Param("attack", 0.99f));
	params.push_back(Param("release", 0.001f));
	break;
    case TYPE_REVERB:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("taps", 1));
	params.push_back(Param("feedback", 0.5f));
	params.push_back(Param("lp", 0.5f));
	break;
    case TYPE_CHORUS:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("modamp", 0.0f));
	params.push_back(Param("time", 0.0f));
	params.push_back(Param("freq", 0.0f));
	break;
    }
    params.resize(4);
}

void Module::from_json(Json::Value &json) {
    params.clear();
    type = name_types.at(json["type"].asString());
    out_op = name_ops.at(json["op"].asString());
    stereo = json["stereo"].asBool();
    comment = json["_comment"].asString();
    if (type == TYPE_OSC) {
	osc_shape = osc_name_shapes.at(json["shape"].asString());
    } else if (type == TYPE_FILTER) {
        filter_type = filter_name_types.at(json.get("filter_type", "lp").asString());
    }
    Json::Value params_json = json["params"];

    switch (type) {
    case TYPE_OSC:
	params.push_back(Param("gain", params_json["gain"].asFloat(), 16));
	params.push_back(Param("freq_mod", params_json["freq_mod"].asFloat()));
	params.push_back(Param("add", params_json["add"].asFloat()));
	params.push_back(Param("detune", params_json["detune"].asFloat()));
	break;
    case TYPE_FILTER:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("cutoff", params_json["cutoff"].asFloat()));
	params.push_back(Param("feedback", params_json["feedback"].asFloat()));
	break;
    case TYPE_ENV:
	params.push_back(Param("switch", params_json["switch"].asFloat()));
	params.push_back(Param("attack", params_json["attack"].asFloat(), 16));
	params.push_back(Param("release", params_json["release"].asFloat()));
	params.push_back(Param("stage", 1));
	break;
    case TYPE_COMP:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("threshold", params_json["threshold"].asFloat(), 16));
	params.push_back(Param("attack", params_json["attack"].asFloat()));
	params.push_back(Param("release", params_json["release"].asFloat()));
	break;
    case TYPE_REVERB:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("taps", params_json["taps"].asInt()));
	params.push_back(Param("feedback", params_json["feedback"].asFloat()));
	params.push_back(Param("lp", params_json["lp"].asFloat()));
	break;
    case TYPE_CHORUS:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("modamp", params_json["modamp"].asFloat()));
	params.push_back(Param("time", params_json["time"].asFloat()));
	params.push_back(Param("freq", params_json["feedback"].asFloat()));
	break;
    default:
	throw TDError("Unrecognized module type: ", json["type"].asString());
    }
    params.resize(4);
    out_module = json["out"].asInt();
    if (json.isMember("out_param")) {
	out_param = json["out_param"].asInt();
    }
    // print();
}

Json::Value Module::as_json() {
    std::cerr << "load module" << std::endl;
    Json::Value json(Json::objectValue);

    json["type"] = type_names.at(type);
    json["out"] = out_module;
    json["op"] = op_names.at(out_op);
    json["stereo"] = stereo;
    json["out_param"] = out_param;
    json["_comment"] = comment;
    if (type == TYPE_OSC) {
	json["shape"] = osc_shape_names[osc_shape];
    } else if (type == TYPE_FILTER) {
        json["filter_type"] = filter_type_names[filter_type];
    }

    Json::Value params_json(Json::objectValue);
    for (const auto &param : params) {
	if (!param.name.empty()) {
	    params_json[param.name] = param.as_json();
	}
    }
    json["params"] = params_json;
    return json;
}

std::vector<Section *> Module::bin() {
    std::cerr << "dump module" << std::endl;
    std::vector<Section *> res;
    std::vector<uint8_t> tmp(4);
    uint32_t out_offset;
    if (out_module < 0) {
	out_offset = 0;
    } else {
	// TODO: turn magic numbers into constants
	out_offset = ((out_module - my_index) * 4 * 5 +
		      out_param * 4);
    }

    res.push_back(new CommentSection("type: " + type2str(type)));
    tmp[0] = static_cast<uint8_t>(type);
    tmp[1] = op_flags[out_op];
    if (stereo) {
	tmp[0] |= 0x80;
    }
    if (type == TYPE_OSC) {
        tmp[2] = osc_shape_flags[osc_shape];
    } else if (type == TYPE_FILTER) {
        tmp[2] = filter_type_flags[filter_type];
    }
    tmp[3] = out_offset;
    res.push_back(new BinSection("modules", tmp));

    for (auto &param : params) {
	res.push_back(new BinSection("modules", param.bin()));
    }

    return res;
}

void Module::print() {
    std::cerr << "-- Module --" << std::endl
	      << " type: " << type_names.at(type) << std::endl
	      << " out_module: " << out_module << std::endl
	      << " out_param: " << out_param << std::endl
	      << " params:";
    for (auto &param : params) {
	if (param.type == Param::TYPE_FLOAT) {
	    std::cerr << " f:" << param.float_value;
	} else {
	    std::cerr << " i:" << param.int_value;
	}
    }
    std::cerr << std::endl << "-------------" << std::endl;
}
