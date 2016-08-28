#include "fonts.hpp"
#include <iostream>

boost::filesystem::path Fonts::font_dir(".");

Fonts::Fonts() {
    TTF_Init();
}

Fonts &Fonts::instance() {
    static Fonts fonts;
    return fonts;
}

TTF_Font *Fonts::load_font(const std::string &filename, int size) {
    boost::filesystem::path full_path(font_dir);
    full_path /= filename;
    std::cerr << "try load " << full_path.native() << " size " << size << std::endl;
    TTF_Font *font = TTF_OpenFont(full_path.c_str(), size);
    return font;
}
