#include "controller.hpp"
#include <algorithm>

Controller::Controller() {
}

Controller::Controller(Type _type, Range _range,
		       double _value_min, double _value_max,
		       int _midi_midpoint, double _value_midpoint,
		       double _initial_midi)
    : type(_type), value_min(_value_min), value_max(_value_max),
      midi_midpoint(_midi_midpoint), value_midpoint(_value_midpoint) {
    int max_midi_value;
    if (_range == RANGE_8BIT) {
	max_midi_value = (2 << 7) - 1;
    } else { // RANGE_14BIT
	max_midi_value = (2 << 14) - 1;
    }
    
    if (midi_midpoint < 0) {
	midi_midpoint = 0;
    } else if (midi_midpoint > max_midi_value) {
	midi_midpoint = max_midi_value;
    }
    
    if (midi_midpoint == 0) {
	range_divisor_lower = 1.0;
    } else {
	range_divisor_lower = double(midi_midpoint);
    }
    if (midi_midpoint == 127) {
	range_divisor_upper = 1.0;
    } else {
	range_divisor_upper = double(max_midi_value - midi_midpoint);
    }

    update(0, _initial_midi);
}

void Controller::update(unsigned long long timestamp, int midi_value) {
    switch (type) {
    case TYPE_LINEAR:
	if (midi_value > midi_midpoint) {
	    midi_value -= midi_midpoint;
	    value = value_midpoint + (value_max - value_midpoint) * midi_value / range_divisor_upper;
	} else {
	    value = value_min + (value_midpoint - value_min) * midi_value / range_divisor_lower;
	}
    }
}
