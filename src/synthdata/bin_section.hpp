#pragma once

#include <vector>
#include <string>
#include <cstdint>

class Section {
public:
    virtual ~Section() {}
    virtual std::string str() = 0;
    std::vector<uint8_t> bin();
};

class BinSection : public Section {
public:
    std::string type;
    std::vector<std::string> comments;
    std::string label;
    std::vector<uint8_t> data;
    BinSection(const std::string &type, const std::string &label = "");
    BinSection(const std::string &type, const std::vector<uint8_t> &data);
    BinSection &add_comment(const std::string &comment);
    std::string str();
};

class DefineSection : public Section {
private:
    std::string name;
    std::string value;
public:
    DefineSection(const std::string &_name, const std::string &_value);
    DefineSection(const std::string &_name, int _value);
    std::string str();
};

class CommentSection : public Section {
private:
    std::vector<std::string> lines;
public:
    CommentSection(const std::string &line);
    CommentSection &add_line(const std::string &line);
    std::string str();
};

class Label : public Section {
private:
    std::string name;
public:
    Label(const std::string &_name);
    std::string str();
};

