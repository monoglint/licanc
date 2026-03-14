#pragma once

#include <string>
#include <stdexcept>

namespace util {
    struct PanicAssertion : public std::runtime_error {
        PanicAssertion(std::string message)
            : std::runtime_error(message)
        {}
    };

    [[noreturn]]
    void panic(std::string message, bool exit = false);

    inline void panic_assert(bool condition, std::string message, bool exit = false) {
        if (!condition)
            panic(message, exit);
    }
}