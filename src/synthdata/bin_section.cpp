#include "bin_section.hpp"
#include <sstream>

std::vector<uint8_t> Section::bin() {
    return std::vector<uint8_t>();
}

BinSection::BinSection(const std::string &type_, const std::string &label_)
    : type(type_), label(label_) {}

BinSection::BinSection(const std::string &type_, const std::vector<uint8_t> &data_)
    : type(type_), data(data_) {}

std::string BinSection::str() {
    std::stringstream res;
    for (auto &comment : comments) {
	res << ";; " << comment << std::endl;
    }
    if (!label.empty()) {
	res << label << ":" << std::endl;
    }
    for (auto &byte : data) {
	res << "db " << static_cast<int>(byte) << std::endl;
    }
    return res.str();
}

BinSection &BinSection::add_comment(const std::string &comment) {
    comments.push_back(comment);
    return *this;
}

DefineSection::DefineSection(const std::string &_name, const std::string &_value)
    : name(_name), value(_value) {}

DefineSection::DefineSection(const std::string &_name, int _value)
    : name(_name) {
    std::stringstream value_str;
    value_str << _value;
    value = value_str.str();
}

std::string DefineSection::str() {
    std::stringstream value_str;
    value_str << "%define " << name << " " << value << std::endl;
    return value_str.str();
}

CommentSection::CommentSection(const std::string &line) {
    add_line(line);
}

CommentSection &CommentSection::add_line(const std::string &line) {
    lines.push_back(line);
    return *this;
}

std::string CommentSection::str() {
    std::stringstream res;
    for (auto &line: lines) {
	res << ";; " << line << std::endl;
    }
    return res.str();
}

Label::Label(const std::string &_name) : name(_name) {}

std::string Label::str() {
    std::stringstream res;
    res << name << ":" << std::endl;
    return res.str();
}
