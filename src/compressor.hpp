#ifndef _COMPRESSOR_H_
#define _COMPRESSOR_H_

class Compressor {
    double threshold = 0.5;
    double attack = 0.99;
    double release = 1.001;

    double amp = 1.0;
public:
    void run(double input[2], double output[2]);
};

#endif // _COMPRESSOR_H_
