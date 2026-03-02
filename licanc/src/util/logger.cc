#include "util/logger.hh"

#include <sstream>

#include "util/ansi_format.hh"

std::string util::t_log::to_string(bool format) const {
    std::stringstream buffer;

    if (format) {
        buffer << util::ansi_format::LIGHT_GRAY << span.start.to_string() << ":\n" << util::ansi_format::RESET;

        switch (log_type) {
            case t_log_type::MESSAGE:
                buffer << util::ansi_format::CYAN;
                break;
            case t_log_type::WARNING:
                buffer << util::ansi_format::YELLOW << util::ansi_format::BOLD << "Warning: ";
                break;
            case t_log_type::ERROR:
                buffer << util::ansi_format::RED << util::ansi_format::BOLD << "Error: ";
                break;
        }
    }
    else {
        buffer << span.start.to_string() << ":\n";
        switch (log_type) {
            case t_log_type::MESSAGE:
                break;
            case t_log_type::WARNING:
                buffer << "Warning: ";
                break;
            case t_log_type::ERROR:
                buffer << "Error: ";
                break;
        }
    }
        
    buffer << message;
    
    if (format)
        buffer << util::ansi_format::RESET;

    return buffer.str();
}

std::string util::t_logger::to_string(bool format) const {
    std::stringstream buffer;

    buffer << "Logs:\n";
    for (const t_log& log : logs) {
        buffer << log.to_string(format) << "\n\n";
    }

    return buffer.str();
}