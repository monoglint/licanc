/*

used for processing the frontend and spitting out something final for the code generator to use

*/

#pragma once

#include "manager_types.hh"

#include <string>
#include <expected>
#include <optional>

#include "frontend/scan/token.hh"
#include "frontend/scan/ast.hh"
#include "frontend/sema/sema.hh"

#include "util/intern_pool.hh"

namespace frontend::manager {
    enum class t_log_type {
        MESSAGE,
        WARNING,
        ERROR,
        INTERNAL_ERROR,
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
        scan::ast::t_ast ast;
        sema::sym::t_symbol_table symbol_table;

        // quick access to all import nodes
        scan::ast::t_node_ids import_node_ids;

        // whether or not the file is the root or has been included
        // used to prevent double inclusion or interdependency
        t_file_state state = t_file_state::PARSE_READY;
    };

    using t_compilation_files = std::deque<t_compilation_file>;

    struct t_log {
        t_log(t_file_id file_id, util::t_span span, t_log_type log_type, std::string message)
            : file_id(file_id), span(span), log_type(log_type), message(message) {}
            
        t_file_id file_id;
        util::t_span span;
        t_log_type log_type;
        std::string message;

        std::string to_string(const t_compilation_files& files, bool format = true) const;
    };

    struct t_logger {
        std::vector<t_log> logs;

        inline void add_log(t_file_id file_id, t_log_type log_type, util::t_span span, std::string message) { logs.emplace_back(file_id, span, log_type, message); }
        
        inline void add_message(t_file_id file_id, util::t_span span, std::string message)          { add_log(file_id, t_log_type::MESSAGE, span, message); }
        inline void add_warning(t_file_id file_id, util::t_span span, std::string message)          { add_log(file_id, t_log_type::WARNING, span, message); }
        inline void add_error(t_file_id file_id, util::t_span span, std::string message)            { add_log(file_id, t_log_type::ERROR, span, message); }
        inline void add_internal_error(t_file_id file_id, util::t_span span, std::string message)   { add_log(file_id, t_log_type::INTERNAL_ERROR, span, message); }

        std::string to_string(const t_compilation_files& files, bool format = true) const;
    };

    // DELIVERY INPUT
    struct t_frontend_config {
        // cd of project_path depends on cd of the program
        std::string project_path;

        // path to the starting point file.
        std::string start_subpath;
        // more info like flags etc.
    };

    struct t_compile_time_data {
        util::t_intern_pool<std::string, t_identifier_id> identifier_pool;
        util::t_intern_pool<std::string, t_string_literal_id> string_literal_pool;

        // not a mistake. only string representations of numbers are stored, not the suffix.
        util::t_intern_pool<std::string, t_number_literal_id> number_literal_pool;

        // the 0th index is not valid. it just refers to a type that needs to be filled later.
        util::t_intern_pool<sema::t_type_name, t_type_name_id, sema::t_type_name_hasher> typename_pool;
    };

    // DELIVERY OUTPUT
    struct t_compilation_unit {
        enum class t_add_file_error {
            FILE_ALREADY_EXISTS,
            PATH_INVALID,
        };
        using t_add_file_result = std::expected<t_file_id, t_add_file_error>; 

        using t_get_file_result = std::optional<std::reference_wrapper<t_compilation_file>>;

        t_compilation_unit(t_frontend_config _config);

        t_frontend_config config;
        t_compile_time_data compile_time_data;
        
        t_logger logger;

        void process_file(t_file_id root_file_id);

        t_add_file_result add_file(std::string path);
        t_get_file_result get_file(t_file_id file_id);
    private:
        t_compilation_files files;
    };
};