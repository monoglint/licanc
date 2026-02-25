#pragma once

#include <string>

namespace util {
    struct t_point {
        u32 char_pos = 0;
        u32 line = 0;
        u32 column = 0;

        std::string to_string() const;
    };

    struct t_span {
        t_point start = t_point();
        t_point end = t_point();

        std::string to_string() const;
    };
}
