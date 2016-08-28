#pragma once

#include <string>
#include <sstream>

class TDError {
public:
    TDError(const std::string &_err) : err(_err) {}
    TDError(const std::string &_err, const std::string &arg) {
	std::stringstream s;
	s << _err << arg;
	err = s.str();
    }
    std::string err;
};
