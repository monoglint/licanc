/*

util:panic

This module provides a readable log to users of a program when an unreachable or unexpected result occurs in code that can not be recovered.
A good example would be instantiating a non-optional pointer with nullptr, which would indicate a problem with the program's source code, and should not be recovered from.

There is, however, an exception option which allows recovery depending on the situation.

@monoglint
30 March 2026

*/
module;

#include <string>
#include <stdexcept>
#include <iostream>

export module util:panic;

import :ansi_format;

export namespace util {
    struct PanicAssertion : public std::runtime_error {
        PanicAssertion(std::string message)
            : std::runtime_error(message)
        {}
    };

    [[noreturn]]
    void panic(std::string message, bool throw_exception = false) {
        std::string title = throw_exception ? "Licanc encountered an error!" : "Licanc has encountered a fatal error!";

        std::string separator = std::string("\n") + util::ansi_format::LIGHT_GRAY + std::string(title.size(), '=') + util::ansi_format::RESET + '\n';

        std::cerr << '\n' << util::ansi_format::RED << util::ansi_format::BOLD << util::ansi_format::UNDERLINE << title << '\n' << util::ansi_format::RESET;
        std::cerr << separator << '\n';
        std::cerr << message << '\n';
        std::cerr << separator << '\n';
        std::cerr << util::ansi_format::LIGHT_BLUE << "Stack trace:\n";
        // std::cerr << std::to_string(std::stacktrace::current()) << util::ansi_format::RESET << '\n';
        std::cerr << "[clang++23 does not support <stacktrace>]\n";
        std::cerr << util::ansi_format::RESET;
        std::cerr << separator << '\n';
        std::cerr << "Please report an issue on the repository and provide this error log.\n";
        
        if (throw_exception)
            throw PanicAssertion(message);

        std::exit(1); 
    }

    void panic_assert(bool condition, std::string message, bool throw_exception = false) {
        if (!condition)
            panic(message, throw_exception);
    }
}