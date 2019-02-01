#pragma once

#include <vector>
#include <string>
#include <json/json.h>
#include "bin_section.hpp"
#include "errors.hpp"

class Module {
private:
    static std::string type2str(int type);
public:
    typedef enum {
	TYPE_OSC = 0,
	TYPE_FILTER = 1,
	TYPE_ENV = 2,
	TYPE_COMP = 3,
	TYPE_REVERB = 4,
	TYPE_CHORUS = 5
    } ModuleType;

    Module();
    Module(Json::Value &json);
    Module(ModuleType type);
    class Param {
    public:
	Param();
	Param(const std::string &name, float value, int trunc_bits = 24);
	Param(const std::string &name, int value);
	Json::Value as_json() const;
	std::vector<uint8_t> bin();
	enum {TYPE_INT32, TYPE_FLOAT} type = TYPE_FLOAT;
	float float_value = 0;
	int int_value = 0;
	std::string name;
	int float_trunc_bits = 24;
    };

    ModuleType type = TYPE_OSC;
    static const std::map<ModuleType, std::string> type_names;
    static const std::map<std::string, ModuleType> name_types;
    // my index in module array, used for calculating module output byte offsets
    int my_index = 0;
    bool stereo = true; // stereo flag
    int out_module = -1; // output module idx, 0 means master out
    int out_param = -1;   // output goes to this param (word) in dest. module
    std::string comment;

    // waveshape, for oscillator only
    typedef enum {OSC_SAW, OSC_SINE, OSC_HSIN, OSC_SQUARE, OSC_NOISE, OSC_NUM_SHAPES} OscShape;
    OscShape osc_shape = OSC_SAW;
    std::map<OscShape, const std::string> osc_shape_names = {
	{OSC_SAW, "saw"},
	{OSC_SINE, "sine"},
	{OSC_HSIN, "hsin"},
	{OSC_SQUARE, "square"},
	{OSC_NOISE, "noise"}
    };
    std::map<const std::string, OscShape> osc_name_shapes = {
	{"saw", OSC_SAW},
	{"sine", OSC_SINE},
	{"hsin", OSC_HSIN},
	{"square", OSC_SQUARE},
	{"noise", OSC_NOISE}
    };
    std::map<OscShape, int> osc_shape_flags = {
	{OSC_SAW, 0x00},
	{OSC_HSIN, 0x01},
	{OSC_SINE, 0x03}, // 0x01 & 0x02 -> *2 _and_ sin()
	{OSC_SQUARE, 0x04},
	{OSC_NOISE, 0x08}
    };

    // filter type, for filter only
    typedef enum {FLT_LP, FLT_HP, FLT_NUM_TYPES} FilterType;
    FilterType filter_type = FLT_LP;
    std::map<FilterType, const std::string> filter_type_names = {
        {FLT_LP, "lp"},
        {FLT_HP, "hp"}
    };
    std::map<const std::string, FilterType> filter_name_types = {
        {"lp", FLT_LP},
        {"hp", FLT_HP}
    };
    std::map<FilterType, int> filter_type_flags = {
        {FLT_LP, 0x00},
        {FLT_HP, 0x01}
    };

    typedef enum {OP_SET, OP_ADD, OP_MULT} OutOp;
    OutOp out_op = OP_SET;
    std::map<OutOp, std::string> op_names = {
	{OP_SET, "set"},
	{OP_ADD, "add"},
	{OP_MULT, "mult"}
    };
    std::map<std::string, OutOp> name_ops = {
	{"set", OP_SET},
	{"add", OP_ADD},
	{"mult", OP_MULT}
    };
    std::map<OutOp, uint8_t> op_flags = {
	{OP_SET, 0x00},
	{OP_ADD, 0x01},
	{OP_MULT, 0x02}
    };

    std::vector <Param> params;

    void from_json(Json::Value &json);
    Json::Value as_json();

    /** Dump module as binary for synth

	Structure of module in memory:

	bytes  contents      type
	----------------------------
	0-3    type & flags     [int]
	4      output offset
	5-7    module param 1   [float]
	8-19   module params 2- [float]

	type byte:
	0 - osc
	1 - filter
	2 - envelope
	3 - dyn. compressor
	4 - reverb

	flag byte:
	0 - mono
	1 - stereo

     */
    std::vector<Section *> bin();

    // Print module info to stderr
    void print();
};
