#include "util/span.hh"

std::string util::t_point::to_string() const {
    return "[Line " + std::to_string(line) + " Col " + std::to_string(column) + "]";
}

std::string util::t_span::to_string() const {
    return '[' + start.to_string() + " " + end.to_string() + ']';
}