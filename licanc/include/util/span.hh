#pragma once

#include <string>

namespace util {
    struct t_point {
        std::size_t char_pos = 0;
        std::size_t line = 0;
        std::size_t column = 0;

        std::string to_string() const;
    };

    struct t_span {
        t_point start = t_point();
        t_point end = t_point();

        std::string to_string() const;
    };
}
