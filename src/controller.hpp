#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <limits>

class Controller {
public:
    typedef enum {CTRL_LINEAR} Type;
private:
    unsigned long long prev_timestamp = std::numeric_limits<unsigned long long>::max();
    int prev_midi_value = 0;
    double value= 0.0;
    double value_min = 0.0;
    double value_max = 1.0;
    Type type = CTRL_LINEAR;
public:
    Controller();
    Controller(double _value_min, double _value_max, Type _type);
    void update(unsigned long long timestamp, int midi_value);
    double get_value();
};

#endif // _CONTROLLER_H_
