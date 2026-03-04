#include "util/panic.hh"

// #include <stacktrace>
#include <iostream>
#include <cstdlib>

#include "util/ansi_format.hh"

void util::panic(std::string message, bool exit) {
    std::string title = "Licanc has encountered a fatal error!";
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
    
    if (exit)
        std::exit(1);
    else
        throw PanicAssertion(message);
}