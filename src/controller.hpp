#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <limits>

class Controller {
public:
    typedef enum {TYPE_LINEAR} Type;
    typedef enum {RANGE_8BIT, RANGE_14BIT} Range;
private:
    unsigned long long prev_timestamp = std::numeric_limits<unsigned long long>::max();
    int prev_midi_value = 0;
    double value = 0.0;
    double value_min = 0.0;
    double value_max = 1.0;
    // If midpoint < 0.0, no midpoint -> controller symmetric around min & max average
    double midpoint = -1.0;
    double range_divisor = 127.0;
    Type type = TYPE_LINEAR;
public:
    Controller();
    Controller(double _initial, double _value_min, double _value_max, Type _type, Range _range);
    Controller(double _initial, double _value_min, double _value_max, double _midpoint,
	       Type _type, Range _range);
    void update(unsigned long long timestamp, int midi_value);
    double get_value() const { return value; }
};

#endif // _CONTROLLER_H_
