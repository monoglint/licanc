#pragma once

#include <string>

namespace util {
    [[noreturn]]
    void panic(std::string message);
}