#include "util/logger.hh"

#include <sstream>

#include "util/ansi_format.hh"

bool util::Logger::has_errors() const {
    return std::find_if(logs.begin(), logs.end(), [](const Log& log) -> bool {
        return log.log_type == LogType::ERROR;
    }) != logs.end();
}

std::string util::Log::to_string(bool format) const {
    std::stringstream buffer;

    if (format) {
        buffer << util::ansi_format::LIGHT_GRAY << span.start.to_string() << ":\n" << util::ansi_format::RESET;

        switch (log_type) {
            case LogType::MESSAGE:
                buffer << util::ansi_format::CYAN;
                break;
            case LogType::WARNING:
                buffer << util::ansi_format::YELLOW << util::ansi_format::BOLD << "Warning: ";
                break;
            case LogType::ERROR:
                buffer << util::ansi_format::RED << util::ansi_format::BOLD << "Error: ";
                break;
        }
    }
    else {
        buffer << span.start.to_string() << ":\n";
        switch (log_type) {
            case LogType::MESSAGE:
                break;
            case LogType::WARNING:
                buffer << "Warning: ";
                break;
            case LogType::ERROR:
                buffer << "Error: ";
                break;
        }
    }
        
    buffer << message;
    
    if (format)
        buffer << util::ansi_format::RESET;

    return buffer.str();
}

std::string util::Logger::to_string(bool format) const {
    std::stringstream buffer;

    buffer << "Logs:\n";
    for (const Log& log : logs) {
        buffer << log.to_string(format) << "\n\n";
    }

    return buffer.str();
}