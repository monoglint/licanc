/*

util:span

This is not as much of a utility as the other structs, but it provides the compiler with a clear, globally-accessible type which grants it a location here.

@monoglint
30 March 2026

*/

module;

#include <cstddef>
#include <string>

export module util:span;

export namespace util {
    struct Point {
        std::size_t char_pos = 0;
        std::size_t line = 0;
        std::size_t column = 0;

        std::string to_string() const {
            return "[Line " + std::to_string(line) + " Col " + std::to_string(column) + "]";
        }
    };

    struct Span {
        Point start = Point();
        Point end = Point();

        std::string to_string() const {
            return '[' + start.to_string() + " " + end.to_string() + ']';
        }
    };
}
