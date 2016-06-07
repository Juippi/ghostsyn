#include "dcfilter.hpp"

double DCFilter::run(double input) {
    double output = input - x + 0.995 * y;
    x = input;
    y = output;
    return output;
}
