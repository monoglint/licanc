/*

used for processing the frontend and spitting out something final for the code generator to use

*/

#pragma once

#include <string>
#include <expected>
#include <optional>

#include "frontend/token.hh"
#include "frontend/ast.hh"
#include "frontend/sym.hh"

#include "util/intern_pool.hh"

namespace frontend::manager {
    using t_file_id = size_t; // index of t_compilation_file in t_compilation_unit::files

    enum class t_log_type {
        MESSAGE,
        WARNING,
        ERROR,
        INTERNAL_ERROR,
    };

    struct t_log {
        t_log(t_file_id file_id, base::t_span span, t_log_type log_type, std::string message)
            : span(span), log_type(log_type), message(message) {}
            
        t_file_id file_id;
        base::t_span span;
        t_log_type log_type;
        std::string message;
    };

    struct t_logger {
        std::vector<t_log> logs;

        inline void add_log(t_file_id file_id, t_log_type log_type, base::t_span span, std::string message) { logs.emplace_back(file_id, span, log_type, message); }
        
        inline void add_message(t_file_id file_id, base::t_span span, std::string message)          { add_log(file_id, t_log_type::MESSAGE, span, message); }
        inline void add_warning(t_file_id file_id, base::t_span span, std::string message)          { add_log(file_id, t_log_type::WARNING, span, message); }
        inline void add_error(t_file_id file_id, base::t_span span, std::string message)            { add_log(file_id, t_log_type::ERROR, span, message); }
        inline void add_internal_error(t_file_id file_id, base::t_span span, std::string message)   { add_log(file_id, t_log_type::INTERNAL_ERROR, span, message); }
    };

    enum class t_file_state {
        PARSE_READY, // includes lexing and parsing
        ANALYZE_READY,
        DONE,
    };

    struct t_compilation_file {
        t_compilation_file(std::string path, std::string source_code)
            : path(std::move(path)), source_code(std::move(source_code)) {}

        std::string path;
        std::string source_code;
        ast::t_ast ast;
        sym::t_symbol_table symbol_table;

        // quick access to all import nodes
        std::vector<ast::t_node_id> import_node_ids;

        // whether or not the file is the root or has been included
        // used to prevent double inclusion or interdependency
        t_file_state state = t_file_state::PARSE_READY;
    };

    // DELIVERY INPUT
    struct t_frontend_config {
        // cd of project_path depends on cd of the program
        std::string project_path;

        // path to the starting point file.
        std::string start_subpath;
        // more info like flags etc.
    };

    // DELIVERY OUTPUT
    struct t_compilation_unit {
        enum class t_add_file_error {
            FILE_ALREADY_EXISTS,
            PATH_INVALID,
        };
        using t_add_file_result = std::expected<t_file_id, t_add_file_error>; 

        using t_get_file_result = std::optional<std::reference_wrapper<t_compilation_file>>;

        t_compilation_unit(t_frontend_config config);

        t_frontend_config config;
        t_logger logger;

        util::t_intern_pool<std::string> identifier_pool;
        util::t_intern_pool<std::string> string_literal_pool;

        void process_file(t_file_id root_file_id);

        t_add_file_result add_file(std::string path);
        t_get_file_result get_file(size_t file_id);
    private:
        std::vector<t_compilation_file> files;
    };
};