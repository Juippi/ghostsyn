#include "controller.hpp"

Controller::Controller() {
}

Controller::Controller(double _initial, double _value_min, double _value_max,
		       double _midpoint, Type _type, Range _range)
    : value(_initial), value_min(_value_min), value_max(_value_max),
      midpoint(_midpoint), type(_type) {
    switch (_range) {
    case RANGE_8BIT:
	range_divisor = 127.0;
	break;
    case RANGE_14BIT:
	range_divisor = 8192.0;
	break;
    }
}

Controller::Controller(double _initial, double _value_min, double _value_max,
		       Type _type, Range _range)
    : Controller(_initial, _value_min, _value_max, -1.0, _type, _range) {
}

void Controller::update(unsigned long long timestamp, int midi_value) {
    switch (type) {
    case TYPE_LINEAR:
	if (midpoint >= 0.0) {
	    if (midi_value > 8192) {
		midi_value -= 8192;
		value = midpoint + (value_max - midpoint) * midi_value / range_divisor;
	    } else {
		value = value_min + (midpoint - value_min) * midi_value / range_divisor;
	    }
	} else {
	    value = value_min + (value_max - value_min) * midi_value / range_divisor;
	}
	break;
    }
}
