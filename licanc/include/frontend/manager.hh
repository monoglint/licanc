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

#include "licanc.hh"

#include "util/intern_pool.hh"

namespace frontend::manager {
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
    };
    
    enum class t_file_state {
        SCAN_READY, 
        SEMA_READY,
        DONE, // this file has had changes to its source code and is ready to be recompiled if needed
    };
    
    struct t_compilation_file {
        t_compilation_file(std::string path, std::string source_code)
            : path(std::move(path)), source_code(std::move(source_code)) {}

        std::string path;
        std::string source_code;
        scan::ast::t_ast ast;

        // a list of files that need to be wiped if this one is during incremental compilation
        // a file can only have moochers if its state is DONE (affirmed in manager.cc/handle_file_imports)
        std::vector<t_file_id> moocher_ids;

        t_logger logger;

        t_file_state state = t_file_state::SCAN_READY;
    };
    
    struct t_compilation_files {
        enum class t_add_file_error {
            FILE_ALREADY_EXISTS,
            PATH_INVALID,
        };

        using t_add_file_result = std::expected<t_file_id, t_add_file_error>; 
        using t_get_file_result = std::optional<std::reference_wrapper<t_compilation_file>>;
        using t_get_const_file_result = std::optional<std::reference_wrapper<const t_compilation_file>>;
        using t_find_file_result = std::optional<t_file_id>;

        t_add_file_result add_file(std::string path);

        [[nodiscard]]
        t_get_file_result get_file(t_file_id file_id);

        [[nodiscard]]
        t_get_const_file_result get_file(t_file_id file_id) const;

        [[nodiscard]]
        t_find_file_result find_file(std::string path) const;

        [[nodiscard]]
        inline std::size_t size() const { return files.size(); }

        // returns true if there is a singular error from at least one file
        [[nodiscard]]
        bool has_errors() const;

    private:
        std::deque<t_compilation_file> files;
    };

    struct t_compile_time_data {
        using t_identifier_pool = util::t_intern_pool<std::string, t_identifier_id>;
        using t_string_literal_pool =util::t_intern_pool<std::string, t_string_literal_id>;
        using t_typename_pool = util::t_intern_pool<sema::t_type_name, t_type_name_id>;

        t_identifier_pool identifier_pool;
        t_string_literal_pool string_literal_pool;
        t_typename_pool typename_pool;
    };

    struct t_frontend_config {
        t_frontend_config(licanc::t_licanc_config& config)
            : project_path(config.project_path), start_subpath(config.start_subpath), target_import_paths(config.target_import_paths)
        {}

        const std::string& project_path;
        const std::string& start_subpath;
        
        const std::vector<std::string>& target_import_paths;
    };

    // to check for errors, run files.has_errors()
    struct t_compilation_unit {
        t_compilation_unit(t_frontend_config _config);

        t_frontend_config config;
        t_compile_time_data compile_time_data;

        sema::sym::t_sym_table sym_table;
        
        t_compilation_files files;

        // compile or recompile the start_subpath file
        void compile();
        
        // compile or recompile a specifc target
        void compile(t_file_id target_file_id);

        void clear_file(t_file_id file_id);
    };

    //

    [[nodiscard]]
    std::string standardize_import_path(std::string project_path, std::string path);
};