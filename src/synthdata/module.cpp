#include "module.hpp"
#include "utils.hpp"
#include <cstring>
#include <iostream>
#include <boost/range/irange.hpp>

using boost::irange;

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
    return float2bin(value, float_trunc_bits);
}

Module::Param::Param() {}

Module::Param::Param(const std::string &name_, float value_, bool keeps_state_,
                     int trunc_bits)
    : name(name_), value(value_), float_trunc_bits(trunc_bits),
      keeps_state(keeps_state_) {}

Json::Value Module::Param::as_json() const {
    return Json::Value(value);
}

bool Module::Param::is_deduplicable() {
    return (float_trunc_bits >= 24 &&
            !keeps_state &&
            !is_connection_target &&
            !is_trigger_target);
}

Module::Module() {}

Module::Module(Json::Value &json) {
    from_json(json);
}

Module::Module(ModuleType type_)
    : type(type_) {
    switch (type) {
    case TYPE_OSC:
	params.push_back(Param("add", 0.0f));
	params.push_back(Param("gain", 0.1f));
	params.push_back(Param("freq_mod", 0.0f));
	params.push_back(Param("detune", 1.0f));
	break;
    case TYPE_FILTER:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("cutoff", 0.5f));
	params.push_back(Param("feedback", 0.0f));
	break;
    case TYPE_ENV:
	params.push_back(Param("stage", 0.0f, true));
	params.push_back(Param("switch", 0.5f));
	params.push_back(Param("release", 0.999f));
	params.push_back(Param("attack", 0.1f));
	break;
    case TYPE_COMP:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("threshold", 0.5f));
	params.push_back(Param("attack", 0.99f));
	params.push_back(Param("release", 0.001f));
	break;
    case TYPE_REVERB: // TODO
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("taps", 1.0f));
	params.push_back(Param("feedback", 0.5f));
	params.push_back(Param("lp", 0.5f));
	break;
    case TYPE_CHORUS: // TODO?
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
	params.push_back(Param("add", params_json["add"].asFloat()));
	params.push_back(Param("gain", params_json["gain"].asFloat(), 16));
	params.push_back(Param("freq_mod", params_json["freq_mod"].asFloat()));
	params.push_back(Param("detune", params_json["detune"].asFloat()));
	break;
    case TYPE_FILTER:
	params.push_back(Param("input", 0.0f));
	params.push_back(Param("cutoff", params_json["cutoff"].asFloat()));
	params.push_back(Param("feedback", params_json["feedback"].asFloat()));
	break;
    case TYPE_ENV:
	params.push_back(Param("stage", 0.0f, true));
	params.push_back(Param("switch", params_json["switch"].asFloat()));
	params.push_back(Param("attack", params_json["attack"].asFloat(), 16));
	params.push_back(Param("release", params_json["release"].asFloat()));
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
    std::cerr << "dump module hdr" << std::endl;
    std::vector<Section *> res;
    std::vector<uint8_t> tmp(6);

    res.push_back(new CommentSection("type: " + type2str(type)));
    // byte 0, module type, flags
    tmp[0] = static_cast<uint8_t>(type);
    if (stereo) {
	tmp[0] |= 0x80;
    }
    if (type == TYPE_OSC) {
        // tmp[0] = osc_shape_flags[osc_shape]; // TODO: doesn't work atm
    } else if (type == TYPE_FILTER) {
        tmp[0] |= filter_type_flags[filter_type];
    }

    // byte 1: output idx to routing array
    tmp[1] = out_workbuf_idx;

    // 2-5: param input indexes
    // note: these should be written in  _reverse_ order here because they're
    // pushed to FPU stack
    for (size_t i : irange(0u, 4u)) {
        // these are byte offsets, at least for now
        tmp[i + 2] = params[3 - i].index_in_workbuf;
    }
    
    res.push_back(new BinSection("modules", tmp));

    return res;
}

void Module::print() {
    std::cerr << "-- Module --" << std::endl;
    std::cerr << std::endl << "-------------" << std::endl;
}
