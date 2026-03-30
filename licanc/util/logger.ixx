/*

util:logger

This utility provides a logger class that can accumulate messages, warnings, and errors, and bulk-print them all when requested.

@monoglint
30 March 2026

*/

module;

#include <string>
#include <sstream>
#include <vector>

export module util:logger;

import :span;
import :ansi_format;

export namespace util {
    enum class LogType {
        MESSAGE,
        WARNING,
        ERROR,
    };

    struct Log {
        Log(util::Span span, LogType log_type, std::string message)
            : span(span), log_type(log_type), message(message) {}
            
        util::Span span;
        LogType log_type;
        std::string message;

        [[nodiscard]]
        std::string to_string(bool should_format = true) const {
            std::stringstream buffer;

            if (should_format) {
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
            
            if (should_format)
                buffer << util::ansi_format::RESET;

            return buffer.str();
        }
    };

    class Logger {
    public:
        std::vector<Log> logs;

        void add_log(LogType log_type, util::Span span, std::string message) { logs.emplace_back(span, log_type, message); }
        
        void add_message(util::Span span, std::string message)                  { add_log(LogType::MESSAGE, span, message); }
        void add_warning(util::Span span, std::string message)                  { add_log(LogType::WARNING, span, message); }
        void add_error(util::Span span, std::string message)                    { add_log(LogType::ERROR, span, message); }

        [[nodiscard]]
        std::string to_string(bool format = true) const {
            std::stringstream buffer;

            buffer << "Logs:\n";
            for (const Log& log : logs) {
                buffer << log.to_string(format) << "\n\n";
            }

            return buffer.str();
        }

        [[nodiscard]]
        bool has_errors() const {
            return std::find_if(logs.begin(), logs.end(), [](const Log& log) -> bool {
                return log.log_type == LogType::ERROR;
            }) != logs.end();
        }

        void clear() {
            logs.clear();
        }
    };
}
