#include "voice.hpp"
#include "globalconfig.hpp"
#include "rt_controls.hpp"
#include <algorithm>

Voice::Voice()
    : osc_ctr(GlobalConfig::max_oscs_per_voice), params(Instrument::NUM_PARAMS),
      osc_pitches(GlobalConfig::max_oscs_per_voice) {
}

void Voice::set_on(int _instrument, int _note, int _octave, const Instrument &_instrument_ref) {
    instrument = _instrument;
    note = _note;
    octave = _octave;
    std::copy(_instrument_ref.params.begin(), _instrument_ref.params.end(),
	      params.begin());
    std::copy(_instrument_ref.osc_pitches.begin(), _instrument_ref.osc_pitches.end(),
	      osc_pitches.begin());
    osc_shape = _instrument_ref.osc_shape;
}

void Voice::set_off() {
    note = -1;
}

void Voice::run_modulation() {
    static const std::map<Instrument::ParameterType, Instrument::ParameterType> modulations = {
	{Instrument::PARAM_FILTER_CUTOFF, Instrument::PARAM_FILTER_DECAY},
	{Instrument::PARAM_VOLUME, Instrument::PARAM_AMP_DECAY},
	{Instrument::PARAM_PITCH, Instrument::PARAM_PITCH_DECAY}
    };
    for (auto &mod : modulations) {
	if (params[mod.first] < GlobalConfig::envelope_min) {
	    params[mod.first] = 0.0;
	} else {
	    params[mod.first] *= params[mod.second];
	}
    }
    params[Instrument::PARAM_PITCH] = std::max(params[Instrument::PARAM_PITCH],
					       params[Instrument::PARAM_PITCH_FLOOR]);
}

double Voice::filter(double in, std::vector<Controller> &rt_controls) {
    const double flt_co = params[Instrument::PARAM_FILTER_CUTOFF];
    const double cutoff = flt_co * rt_controls[RT_FILTER_CUTOFF].get_value();
    const double fb_amt = (params[Instrument::PARAM_FILTER_FEEDBACK] *
			   (cutoff * 3.296875f - 0.00497436523438f));
    const double feedback = fb_amt * (flt_p1 - flt_p2);
    flt_p1 = (in * cutoff +
	      flt_p1 * (1 - cutoff) +
	      feedback +
	      std::numeric_limits<double>::min());
    flt_p2 = (flt_p1 * cutoff * 2 +
	      flt_p2 * (1 - cutoff * 2) +
	      std::numeric_limits<double>::min());
    
    return flt_p2;
}

double Voice::oscillator(std::vector<Controller> &rt_controls) {
    double osc_out = 0;
    for (size_t i = 0; i < osc_pitches.size(); ++i) {
	if (osc_shape == Instrument::NOISE) {
	    osc_out += (rand() % (1 << 16) - (1 << 15)) / 32768.0;
	} else {
	    if (osc_pitches[i] != 0) {
		osc_ctr[i] += ((notetable[note + 1]) *
			       (1 << octave) / 128 *
			       osc_pitches[i] * params[Instrument::PARAM_PITCH]) *
		    rt_controls[RT_PITCH_BEND].get_value();
		int32_t osc_int = (osc_ctr[i] >> 1) - (1 << 30);
		if (osc_shape == Instrument::SQUARE1) {
		    osc_int &= 0x80000000;
		} else if (osc_shape == Instrument::SQUARE2) {
		    osc_int &= 0xc0000000;
		}
		osc_out += osc_int / (double(1LL << 32));
	    }
	}
    }
    return osc_out * params[Instrument::PARAM_VOLUME];
}

double Voice::run(std::vector<Controller> &rt_controls) {
    double osc = oscillator(rt_controls);
    return filter(osc, rt_controls);
}
