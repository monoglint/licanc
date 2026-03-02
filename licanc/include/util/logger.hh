#pragma once

#include <string>
#include <vector>
#include "util/span.hh"

namespace util {
    enum class t_log_type {
        MESSAGE,
        WARNING,
        ERROR,
    };

    struct t_log {
        t_log(util::t_span span, t_log_type log_type, std::string message)
            : span(span), log_type(log_type), message(message) {}
            
        util::t_span span;
        t_log_type log_type;
        std::string message;

        [[nodiscard]]
        std::string to_string(bool format = true) const;
    };

    struct t_logger {
        std::vector<t_log> logs;

        inline void add_log(t_log_type log_type, util::t_span span, std::string message) { logs.emplace_back(span, log_type, message); }
        
        inline void add_message(util::t_span span, std::string message)                  { add_log(t_log_type::MESSAGE, span, message); }
        inline void add_warning(util::t_span span, std::string message)                  { add_log(t_log_type::WARNING, span, message); }
        inline void add_error(util::t_span span, std::string message)                    { add_log(t_log_type::ERROR, span, message); }

        [[nodiscard]]
        std::string to_string(bool format = true) const;

        [[nodiscard]]
        bool has_errors() const;

        inline void clear() {
            logs.clear();
        }
    };
}
