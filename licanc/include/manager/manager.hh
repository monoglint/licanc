#pragma once

#include <string>
#include <expected>
#include <optional>

#include "frontend/scan/token.hh"
#include "frontend/scan/ast.hh"
#include "frontend/sema/sema.hh"

#include "util/intern_pool.hh"
#include "manager/manager_types.hh"

namespace manager {
    enum class t_file_state {
        SCAN_READY, 
        SEMA_READY,
        DONE, // this file has had changes to its source code and is ready to be recompiled if needed
    };

    // temporary "cache" data that the scheduler uses to target dirty files for recompilation
    struct t_file_dependency_data {
        // a list of files that need to be wiped if this one is during incremental compilation
        // added to in handle_file_imports()
        // cleared from in recompile_dirty_files()
        std::vector<t_file_id> dependent_ids;

        // added to in recurse_mark_dirty()
        // cleared from in recompile_dirty_files()
        std::vector<t_file_id> dirty_dependency_ids;

        /*
        
        used as a temporary marker to prevent duplication if t_frontend_files::recurse_mark_dirty
        can also be used as general dirty identification after a recompilation is called and before the state machine runs
        
        set to true recurse_mark_dirty() by t_frontend_files
        set to false in recompile_dirty_files()

        */
        bool is_dirty = false;

        // call when a dependency needs to be deregistered for now being clean
        // general utility function that does not have to be explicitly called
        // this only skips an std::erase_if
        void remove_dirty_dependency(t_file_id now_clean_dependency_id);
    };

    struct t_frontend_pass_data {
        frontend::scan::token::t_tokens tokens;
        frontend::scan::ast::t_ast ast;
        frontend::sema::sym::t_sym_table sym_table;
    };

    struct t_backend_pass_data {

    };

    struct t_compiler_output_data {
        t_frontend_pass_data frontend;
        t_backend_pass_data backend;

        inline void clear() {
            frontend = {};
            backend = {};
        }
    };
    
    struct t_compilation_file {
        t_compilation_file(std::string path, std::string source_code)
            : path(std::move(path)), source_code(std::move(source_code)) {}

        // "inputs"
        std::string path;
        std::string source_code;

        t_compiler_output_data compiler_output_data;

        t_logger logger;

        // scheduling and compilation
        t_file_state state = t_file_state::SCAN_READY;
        t_file_dependency_data dependency_data;

        // check if the source code has been modified since the last update to this file
        // returns true if the source code has been changed since the last refresh
        bool refresh_source_code();
    };
    
    struct t_file_manager {
        enum class t_add_file_error {
            FILE_ALREADY_EXISTS,
            COULDNT_OPEN_FILE,
            PATH_INVALID,
        };

        enum class t_delist_file_result {
            SUCCESS,
            FILE_DOESNT_EXIST,
        };

        using t_add_file_result = std::expected<t_file_id, t_add_file_error>; 
        using t_get_file_result = std::optional<std::reference_wrapper<t_compilation_file>>;
        using t_get_const_file_result = std::optional<std::reference_wrapper<const t_compilation_file>>;
        using t_find_file_result = std::optional<t_file_id>;

        t_file_manager() = default;
        t_file_manager(const t_file_manager&) = delete;
        t_file_manager(t_file_manager&&) = delete;

        std::deque<t_file_id> dirty_files;

        // should be called internally to register a new file in a project.
        t_add_file_result add_file(std::string path);

        t_delist_file_result delist_file(t_file_id file_id);

        [[nodiscard]]
        t_get_file_result get_file(t_file_id file_id);

        [[nodiscard]]
        t_get_const_file_result get_file(t_file_id file_id) const;

        [[nodiscard]]
        t_find_file_result find_file(std::string path) const;

        [[nodiscard]]
        inline std::size_t size() const { return files.size(); }

        [[nodiscard]]
        std::vector<t_file_id> get_valid_files() const;

        // returns true if there is a singular error from at least one file
        [[nodiscard]]
        bool has_errors() const;

    private:
        using t_file_entry = std::optional<t_compilation_file>; 
        std::deque<t_file_entry> files;
    };

    struct t_scheduler {
        t_file_manager& file_manager;

        void recurse_mark_dirty(t_file_id start);
        
        // mark files dirty and remove deleted files
        void refresh_files();

        // call after refresh_files()
        void recompile_dirty_files();

        void run_compiler_state_machine(t_file_id target_file_id);
    };

    // a central global arrangement of pools for parts of the compiler to feed to
    struct t_session_pools {
        using t_identifier_pool = util::t_intern_pool<std::string, t_identifier_id>;
        using t_string_literal_pool = util::t_intern_pool<std::string, t_string_literal_id>;
        using t_typename_pool = util::t_intern_pool<frontend::sema::t_type_name, t_type_name_id>;

        t_identifier_pool identifier_pool;
        t_string_literal_pool string_literal_pool;
        t_typename_pool typename_pool;
    };

    struct t_compilation_engine {
        t_session_pools session_pools;
        t_file_manager file_manager;
        t_scheduler scheduler;

        // compile or recompile the start_subpath file
        void compile();
    };
}