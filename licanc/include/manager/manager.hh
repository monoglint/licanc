#pragma once

#include <string>
#include <expected>
#include <optional>

#include "frontend/scan/tok.hh"
#include "frontend/scan/ast.hh"
#include "frontend/sema/sema.hh"
#include "frontend/sema/resolved_type.hh"

#include "util/intern_pool.hh"
#include "util/logger.hh"
#include "manager/manager_types.hh"

namespace manager {
    enum class FileState {
        SCAN_READY, 
        SEMA_READY,
        DONE, // this file has had changes to its source code and is ready to be recompiled if needed
    };

    // temporary "cache" data that the file_refresher uses to target dirty files for recompilation
    struct FileDependencyData {
        // a list of files that need to be wiped if this one is during incremental compilation
        // added to in handle_file_imports()
        // cleared from in recompile_dirty_files()
        std::vector<FileId> dependent_ids;

        // added to in recurse_mark_dirty()
        // cleared from in recompile_dirty_files()
        std::vector<FileId> dirty_dependency_ids;

        /*
        
        used as a temporary marker to prevent duplication if FrontendFiles::recurse_mark_dirty
        can also be used as general dirty identification after a recompilation is called and before the state machine runs
        
        set to true recurse_mark_dirty() by FrontendFiles
        set to false in recompile_dirty_files()

        */
        bool is_dirty = false;

        // call when a dependency needs to be deregistered for now being clean
        // general utility function that does not have to be explicitly called
        // this only skips an std::erase_if
        void remove_dirty_dependency(FileId now_clean_dependency_id);
    };

    struct FrontendPassData {
        frontend::scan::tok::Tokens tokens;
        frontend::scan::ast::AST ast;
        frontend::sema::sym::SymTable sym_table;

        inline void clear() { tokens.clear(); ast.clear(); sym_table.clear(); }
    };

    struct BackendPassData {
        inline void clear() { }
    };

    struct CompilerOutputData {
        FrontendPassData frontend;
        BackendPassData backend;

        // ALERT ALERT!!! MOST USEFUL FUNCTION IN THE ENTIRE COMPILER RIGHT HERE!!! ALERT!!
        inline void clear() {
            frontend.clear();
            backend.clear();
        }
    };
    
    class CompilationFile {
    public:
        CompilationFile(std::string path, std::string source_code)
            : path(std::move(path)), source_code(std::move(source_code)) {}

        // "inputs"
        std::string path;
        std::string source_code;

        CompilerOutputData compiler_output_data;

        util::Logger logger;

        // scheduling and compilation
        FileState state = FileState::SCAN_READY;
        FileDependencyData dependency_data;

        // check if the source code has been modified since the last update to this file
        // returns true if the source code has been changed since the last refresh
        bool refresh_source_code();
    };
    
    // calling any method in this struct is not file_refresher safe. always interface with the file_refresher if accessing externally
    class FileManager {
    public:
        enum class AddFileError {
            FILE_ALREADY_EXISTS,
            COULDNT_OPEN_FILE,
            PATH_INVALID,
        };

        enum class DelistFileResult {
            SUCCESS,
            FILE_DOESNT_EXIST,
        };

        using AddFileResult = std::expected<FileId, AddFileError>;
        using GetFileResult = std::optional<std::reference_wrapper<CompilationFile>>;
        using GetConstFileResult = std::optional<std::reference_wrapper<const CompilationFile>>;
        using FindFileResult = std::optional<FileId>;

        FileManager() = default;
        FileManager(const FileManager&) = delete;
        FileManager(FileManager&&) = delete;

        // should be called internally to register a new file in a project.
        AddFileResult add_file(std::string path);

        DelistFileResult delist_file(FileId file_id);

        [[nodiscard]]
        GetFileResult get_file(FileId file_id);

        [[nodiscard]]
        GetConstFileResult get_file(FileId file_id) const;

        [[nodiscard]]
        FindFileResult find_file(std::string path) const;

        [[nodiscard]]
        inline std::size_t size() const { return files.size(); }

        [[nodiscard]]
        std::vector<FileId> get_valid_files() const;

        // returns true if there is a singular error from at least one file
        [[nodiscard]]
        bool has_errors() const;

    private:
        std::deque<std::optional<CompilationFile>> files;
    };

    class FileRefresher {        
    public:
        FileManager& file_manager;

        std::deque<FileId> dirty_files;

        // mark a file and all of its ancestor-dependents dirty
        void recurse_mark_dirty(FileId start);

        FileManager::DelistFileResult delist_file(FileId file_id);
        
        // mark files dirty and remove deleted files
        void refresh_files();
    };

    // a central global arrangement of pools for parts of the compiler to feed to
    struct SessionPools {
        using IdentifierPool = util::InternPool<std::string, IdentifierId>;
        using StringLiteralPool = util::InternPool<std::string, StringLiteralId>;
        using ResolvedTypePool = util::InternPool<frontend::sema::ResolvedType, ResolvedTypeId>;

        IdentifierPool identifier_pool;
        StringLiteralPool string_literal_pool;
        ResolvedTypePool resolved_type_pool;
    };

    struct EngineContext {
        const std::string& project_path;
        const std::string& start_path;
    };

    class CompilationEngine {
    public:
        CompilationEngine(EngineContext& engine_context)
            : engine_context(engine_context), file_refresher(file_manager)
        {}

        EngineContext engine_context;

        SessionPools session_pools;
        FileManager file_manager;
        FileRefresher file_refresher;

        // compile the start path file or recompile dirty files in the project
        void compile();

        // check for any dirty files and target recompile those
        void recompile_dirty_files();

    private:
        void compile_to_completion(FileId target_file_id);
    };
}