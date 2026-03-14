#pragma once

#include <string>

namespace util {
    struct Point {
        std::size_t char_pos = 0;
        std::size_t line = 0;
        std::size_t column = 0;

        std::string to_string() const;
    };

    struct Span {
        Point start = Point();
        Point end = Point();

        std::string to_string() const;
    };
}
