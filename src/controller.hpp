#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <limits>

class Controller {
public:
    typedef enum {TYPE_LINEAR} Type;
    typedef enum {RANGE_8BIT, RANGE_14BIT} Range;
private:
    unsigned long long prev_timestamp = std::numeric_limits<unsigned long long>::max();

    Type type = TYPE_LINEAR;

    double value_min = 0.0;
    double value_max = 1.0;
    
    int midi_midpoint = 63;
    double value_midpoint = 0.5;
       
    double range_divisor_upper = 127.0;
    double range_divisor_lower = 127.0;

    // Current output value of controller
    double value = 0.0;
public:
    Controller();
    Controller(Type _type, Range _range,
	       double _value_min, double _value_max,
	       int _midi_midpoint, double _value_midpoint,
	       double _initial_midi);
    void update(unsigned long long timestamp, int midi_value);
    double get_value() const { return value; }
};

#endif // _CONTROLLER_H_
