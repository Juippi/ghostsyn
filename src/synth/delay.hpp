#ifndef _DELAY_H_
#define _DELAY_H_

#include <vector>
#include <cstdlib>

class Delay {
    static const size_t delay_length_max = 20000;

    std::vector<double> buffers[2];
    size_t buffer_pos[2] {0, 0};

    size_t delay[2] {20000, 18000};
    double feedback = 0.3;
public:
    Delay();
    void run(double input[2], double output[2]);
};

#endif // _DELAY_H_
