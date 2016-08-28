#ifndef _INSTRUMENTS_H_
#define _INSTRUMENTS_H_
#include <list>
#include <vector>
#include <json/json.h>
#include <cstdint>

class Instruments {
public:

    static const int max_params = 6;
    class Module {
    public:
	class Param {
	public:
	    enum {TYPE_INT32,
		  TYPE_FLOAT} type;
	    float float_value;
	    int int_value;
	    Param(const int &val);
	    Param(const double &val);
	};
	
	std::vector<Param> params;

	enum {TYPE_OSC = 0,
	      TYPE_FILTER = 1,
	      TYPE_ENV = 2,
	      TYPE_COMPRESSOR = 3,
	      TYPE_DELAY = 4};

	enum {
	    FLAG_STEREO = 0x80
	};

	enum {OP_SET = 0,
	      OP_ADD = 1,
	      OP_MULT = 2};

	enum {SHAPE_TRI = 0,
	      SHAPE_SIN = 1};
    };

    int num_modules = 0;
    std::vector<Module> modules;

    // lists of things triggered/configured by tracker
    // each instrument should have 1 osc and env with "tracker_ctl": true.
    // values here are byte offsets of the control point form module list start.
    std::vector<int> env_triggers;
    std::vector<int> osc_pitches;

    int map_out(size_t instrument_off, int out_idx, int tgt_param_idx);
    void load(const std::string filename);
    std::vector<uint8_t> bin();
};

#endif // _INSTRUMENTS_H_
