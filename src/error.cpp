#include <lzxd/error.hpp>
#include <iostream>

#ifdef _WIN32
# include <Windows.h> // for DebugBreak
#else
# include <csignal> // for raise
#endif

namespace lzxd {

void _assertfail(const char* message, const char* file, int line) {
    std::cerr << "Assertion failed: " << message << " at " << file << ":" << line << std::endl;

    // Breakpoint
#ifdef _WIN32
    DebugBreak();
#else
    raise(SIGTRAP);
#endif

    // Unreachable code
    std::abort();
}

} // namespace lzxd