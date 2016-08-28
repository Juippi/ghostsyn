#pragma once

#include <cstdint>

class Color {
public:
    uint8_t r, g, b, a;
    Color(uint8_t r_ = 0, uint8_t g_ = 0, uint8_t b_ = 0, uint8_t a_ = 255)
	: r(r_), g(g_), b(b_), a(a_) {}
};

class Rect {
public:
    int x, y, w, h;
    Rect(int x_ = 0, int y_ = 0, int w_ = 0, int h_ = 0)
	: x(x_), y(y_), w(w_), h(h_) {}
};
