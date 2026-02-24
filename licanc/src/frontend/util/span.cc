#include "util/span.hh"

std::string util::t_point::to_string() const {
    return "[Line " + std::to_string(line) + " Col " + std::to_string(column) + "]";
}

util::t_span::t_span(t_point start, t_point end)
    : start(std::move(start)), end(std::move(end)) {}

std::string util::t_span::to_string() const {
    return '[' + start.to_string() + " " + end.to_string() + ']';
}