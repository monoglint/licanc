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
        DEFAULT = 39,
        BACK_BLACK = 40,
        BACK_RED = 41,
        BACK_GREEN = 42,
        BACK_YELLOW = 43,
        BACK_BLUE = 44,
        BACK_MAGENTA = 45,
        BACK_CYAN = 46,
        BACK_WHITE = 47,
        BACK_DEFAULT = 49,
        LIGHT_GRAY = 90,
        LIGHT_RED = 91,
        LIGHT_GREEN = 92,
        LIGHT_YELLOW = 93,
        LIGHT_BLUE = 94,
        LIGHT_MAGENTA = 95,
        LIGHT_CYAN = 96,
        LIGHT_WHITE = 97,
    };

    template <t_format FORMAT, class T_CHAR, class T_TRAITS>
    std::basic_ostream<T_CHAR, T_TRAITS>& t_stream_format(std::basic_ostream<T_CHAR, T_TRAITS>& ostream) {
        ostream << "\033[" << static_cast<int>(FORMAT) << "m";

        return ostream;
    }
}