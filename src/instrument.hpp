#ifndef _INSTRUMENT_H_
#define _INSTRUMENT_H_
#include <vector>
#include <map>
#include <string>
#include <istream>

class Instrument {
public:
    enum ParameterType {
	PARAM_VOLUME,
	PARAM_AMP_DECAY,

	PARAM_FILTER_CUTOFF,
	PARAM_FILTER_DECAY,
	PARAM_FILTER_FEEDBACK,

	PARAM_PITCH,
	PARAM_PITCH_DECAY,
	PARAM_PITCH_FLOOR,

	NUM_PARAMS
    };

    // Map parameter names to parameters (for patch loader).
    const std::map<std::string, ParameterType> name_param_map {
	{"volume", PARAM_VOLUME},
	    
	{"filter_cutoff", PARAM_FILTER_CUTOFF},
	{"filter_decay", PARAM_FILTER_DECAY},
	{"filter_feedback", PARAM_FILTER_FEEDBACK},

	{"amp_decay", PARAM_AMP_DECAY},
	    
	{"pitch", PARAM_PITCH},
	{"pitch_decay", PARAM_PITCH_DECAY},
	{"pitch_min", PARAM_PITCH_FLOOR},
    };

    std::vector<double> params;

    enum OscShape {SAW, TRIANGLE, NOISE, SQUARE1, SQUARE2};
    OscShape osc_shape;
    std::vector<double> osc_pitches;

    Instrument();
    void load(std::istream &in_stream);
};

#endif // _INSTRUMENT_H_
