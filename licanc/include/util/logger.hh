#pragma once

#include <string>
#include <vector>
#include "util/span.hh"

namespace util {
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
        std::string to_string(bool format = true) const;
    };

    class Logger {
    public:
        std::vector<Log> logs;

        inline void add_log(LogType log_type, util::Span span, std::string message) { logs.emplace_back(span, log_type, message); }
        
        inline void add_message(util::Span span, std::string message)                  { add_log(LogType::MESSAGE, span, message); }
        inline void add_warning(util::Span span, std::string message)                  { add_log(LogType::WARNING, span, message); }
        inline void add_error(util::Span span, std::string message)                    { add_log(LogType::ERROR, span, message); }

        [[nodiscard]]
        std::string to_string(bool format = true) const;

        [[nodiscard]]
        bool has_errors() const;

        inline void clear() {
            logs.clear();
        }
    };
}
