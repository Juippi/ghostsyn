#include "controller.hpp"

Controller::Controller() {
}

Controller::Controller(double _value_min, double _value_max, Type _type)
    : value_min(_value_min), value_max(_value_max), type(_type) {
}

void Controller::update(unsigned long long timestamp, int midi_value) {
    switch (type) {
    case CTRL_LINEAR:
	value = value_min + (value_max - value_min) * midi_value / 127.0;
	break;
    default:
	break;
    }
}

double Controller::get_value() {
    return value;
}
