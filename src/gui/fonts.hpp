#pragma once

#include <boost/filesystem.hpp>
#include <string>
#include <SDL_ttf.h>

class Fonts {
public:
    Fonts();
    static Fonts &instance();
    static boost::filesystem::path font_dir;
    static TTF_Font *load_font(const std::string &filename, int size);
};
