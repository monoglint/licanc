#pragma once

#include <string>

namespace util {
    struct t_point {
        t_point(u32 char_pos = 0, u32 line = 0, u32 column = 0)
            : char_pos(char_pos), line(line), column(column) {}

        u32 char_pos;
        u32 line;
        u32 column;

        std::string to_string() const;
    };

    struct t_span {
        t_span(t_point start = 0, t_point end = 0);
            
        t_point start;
        t_point end;

        std::string to_string() const;
    };
}
