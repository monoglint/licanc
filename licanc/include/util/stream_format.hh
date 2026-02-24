#pragma once

#include <iostream>

namespace util {
    enum class t_format {
        RESET = 0,
        BOLD = 1,
        UNDERLINE = 4,
        BLACK = 30,
        RED = 31,
        GREEN = 32,
        YELLOW = 33,
        BLUE = 34,
        MAGENTA = 35,
        CYAN = 36,
        WHITE = 37,
    };

    template <t_format FORMAT>
    struct t_stream_format {

    };

    template <class CHAR_T, class TRAITS, t_format FORMAT>
    std::basic_ostream<CHAR_T, TRAITS>& operator<<(std::basic_ostream<CHAR_T, TRAITS>& ostream, t_stream_format<FORMAT> format) {
        ostream << "\033[" << static_cast<int>(FORMAT) << "m";

        return ostream;
    }
}