#pragma once

#include <stdexcept>

#define LZXD_ASSERT(cond) if (!(cond)) lzxd::_assertfail(#cond, __FILE__, __LINE__)

namespace lzxd {

class LzxdError : public std::runtime_error {
public:
    LzxdError(const std::string& message) : std::runtime_error(message) {}
};

[[noreturn]] void _assertfail(const char* message, const char* file, int line);

}