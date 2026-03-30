/*

util:ansi_format

This utility carries many useful constants for adding ANSI encoding to strings being printed to the console.

@monoglint
30 Mar 2026

*/

export module util:ansi_format;

export namespace util::ansi_format {
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* UNDERLINE = "\033[4m";
    constexpr const char* BLACK = "\033[30m";
    constexpr const char* RED = "\033[31m";
    constexpr const char* GREEN = "\033[32m";
    constexpr const char* YELLOW = "\033[33m";
    constexpr const char* BLUE = "\033[34m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* CYAN = "\033[36m";
    constexpr const char* WHITE = "\033[37m";
    constexpr const char* DEFAULT = "\033[39m";
    constexpr const char* BACK_BLACK = "\033[40m";
    constexpr const char* BACK_RED = "\033[41m";
    constexpr const char* BACK_GREEN = "\033[42m";
    constexpr const char* BACK_YELLOW = "\033[43m";
    constexpr const char* BACK_BLUE = "\033[44m";
    constexpr const char* BACK_MAGENTA = "\033[45m";
    constexpr const char* BACK_CYAN = "\033[46m";
    constexpr const char* BACK_WHITE = "\033[47m";
    constexpr const char* BACK_DEFAULT = "\033[49m";
    constexpr const char* LIGHT_GRAY = "\033[90m";
    constexpr const char* LIGHT_RED = "\033[91m";
    constexpr const char* LIGHT_GREEN = "\033[92m";
    constexpr const char* LIGHT_YELLOW = "\033[93m";
    constexpr const char* LIGHT_BLUE = "\033[94m";
    constexpr const char* LIGHT_MAGENTA = "\033[95m";
    constexpr const char* LIGHT_CYAN = "\033[96m";
    constexpr const char* LIGHT_WHITE = "\033[97m";
}