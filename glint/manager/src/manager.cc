module;

#include <fstream>
#include <filesystem>
#include <expected>
#include <iostream>
#include <algorithm>
#include <string>

module manager;

import util;

import frontend.lex_pass;
import frontend.parse_pass;
import frontend.sema_pass;

namespace {
    enum class QuickReadFileError {
        COULDNT_OPEN_FILE,
        PATH_INVALID
    };

    using QuickReadFileResult = std::expected<std::string, QuickReadFileError>;
    QuickReadFileResult quick_read_file(std::string file_path) {
        if (!std::filesystem::exists(file_path))
            return std::unexpected(QuickReadFileError::PATH_INVALID);

        std::ifstream input_file(file_path);

        if (!input_file || !input_file.is_open())
            return std::unexpected(QuickReadFileError::COULDNT_OPEN_FILE);

        return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());    
    }
}

namespace {
    // used by handle_file_imports to mark the current file as an added or existing file's dependency
    // returns the file_id of the new dependency of the given file
    /*
    
    file_id -> file_id of the file that is now a registered dependent of the file who's path is "absolute_file_path"
    
    */
    manager_types::FileId register_file_dependency(manager::FileManager& file_manager, manager_types::FileId file_id, const std::string& absolute_file_path) {
        manager::FileManager::AddFileResult add_file_result = file_manager.add_file(absolute_file_path);
        
        if (add_file_result.has_value()) {
            manager_types::FileId new_file_id = add_file_result.value();
            manager::CompilationFile& new_file = file_manager.get_file(new_file_id).value();

            new_file.dependency_data.dependent_ids.push_back(file_id);

            return new_file_id;
        }

        else if (add_file_result.error() == manager::FileManager::AddFileError::FILE_ALREADY_EXISTS) {
            manager_types::FileId found_file_id = file_manager.find_file(absolute_file_path).value();
            manager::CompilationFile& found_file = file_manager.get_file(found_file_id).value();
            
            found_file.dependency_data.dependent_ids.push_back(file_id);

            return found_file_id;
        }
        
        util::panic("An unreachable condition was reached by the file dependency registerer. Whether or not 'absolute_file_path' is valid was checked by the file dependency register's only caller.");
    }

    bool detect_file_interdependency(const manager::FileManager& file_manager, manager_types::FileId current_file_id, std::vector<manager_types::FileId>& file_stack, const std::string& absolute_file_path) {
        return std::find_if(file_stack.begin(), file_stack.end(), [&](manager_types::FileId frame_file_id) -> bool {
            if (current_file_id == frame_file_id)
                return false;

            manager::FileManager::GetConstFileResult frame_get_file_result = file_manager.get_file(frame_file_id);
            const manager::CompilationFile& frame_file = frame_get_file_result.value();

            return frame_file.path == absolute_file_path;

        }) != file_stack.end();
    }
    /*

    lazily loading in all of the files we need should occur after parsing because we have just enough information to know what files we need to use.

    the semantic analyzer will take care of linking references between files together, but actually loading included files can occur
    after parsing. this lessens complexity
    
    */
    void handle_post_parse_file_imports(manager::CompilationEngine& engine, manager_types::FileId current_file_id, manager::CompilationFile& file, std::vector<manager_types::FileId>& file_stack) {
        // check if there is an include item anywhere in the ast.
        for (frontend::ast::ImportDecl* import_decl_node : file.compiler_output_data.frontend.ast.import_nodes) {
            if (!import_decl_node->is_path_valid)
                continue;

            manager::SessionPools::StringLiteralPool::ConstGetResult get_string_result = 
                engine.session_pools.string_literal_pool.get(import_decl_node->absolute_file_path->string_literal_id);

            const std::string& absolute_file_path = get_string_result.value();

            bool interdependency_found = detect_file_interdependency(engine.file_manager, current_file_id, file_stack, absolute_file_path);
            if (interdependency_found) {
                file.logger.add_error(import_decl_node->span, std::string("Including \\" + absolute_file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                continue;
            }

            manager_types::FileId dependency_file_id = register_file_dependency(engine.file_manager, current_file_id, absolute_file_path);

            import_decl_node->resolved_file_id = dependency_file_id;
            file_stack.emplace_back(dependency_file_id);
        }
    }

    void parse_file(manager::CompilationEngine& engine, manager_types::FileId file_id, std::vector<manager_types::FileId>& file_stack) {
        manager::CompilationFile& file = engine.file_manager.get_file(file_id).value();

        // frontend::lex_pass::lex(frontend::lex_pass::LexerContext{});
        // frontend::parse_pass::parser::parse(frontend::parse_pass::parser::ParserContext{
        //     .ast = file.compiler_output_data.frontend.ast,
        //     .engine_context = engine.engine_context
        // });

        handle_post_parse_file_imports(engine, file_id, file, file_stack);
    }

    void analyze_file(manager::CompilationEngine& engine, manager_types::FileId file_id) {
        // frontend::sema::semantic_analyzer::analyze(engine, file_id);
    }
}

void manager::FileDependencyData::remove_dirty_dependency(manager_types::FileId now_clean_dependency_id) {
    std::erase_if(dirty_dependency_ids, [&](const manager_types::FileId dirty_dependency_id) -> bool {
        return dirty_dependency_id == now_clean_dependency_id;
    });
}

bool manager::CompilationFile::refresh_source_code() {
    QuickReadFileResult quick_read_file_result = quick_read_file(path);

    if (!quick_read_file_result.has_value()) {
        std::cerr << "Licanc was unable to open \"" 
            << path 
            << "\" to compare to a cached version. The compiler will not register any changes made to the file since the last compilation.\n";

        return false;
    }
    
    if (quick_read_file_result.value() == source_code)
        return false;
    
    source_code = quick_read_file_result.value();

    return true;
}

manager::FileManager::AddFileResult manager::FileManager::add_file(std::string path) {
    const bool file_exists = std::find_if(files.begin(), files.end(), [&path](const FileEntry& file) -> bool {
        return file.is_alive() && file.get_file().path == path;
    }) != files.end();
    
    if (file_exists)
        return std::unexpected(AddFileError::FILE_ALREADY_EXISTS);
    
    QuickReadFileResult quick_read_file_result = quick_read_file(path);

    if (!quick_read_file_result.has_value()) {
        switch (quick_read_file_result.error()) {
            case QuickReadFileError::COULDNT_OPEN_FILE:
                return std::unexpected(AddFileError::COULDNT_OPEN_FILE);
            case QuickReadFileError::PATH_INVALID:
                return std::unexpected(AddFileError::PATH_INVALID);
        }
    }

    files.emplace_back(std::make_unique<CompilationFile>(std::filesystem::absolute(path).string(), quick_read_file_result.value()));

    return manager_types::FileId{files.size() - 1};
}

manager::FileManager::DelistFileResult manager::FileManager::delist_file(manager_types::FileId file_id) {
    if (file_id.get() >= files.size())
        return DelistFileResult::FILE_DOESNT_EXIST;

    // intentionally not throwing an error for if the file was already delisted because i don't really know why i should
    
    files[file_id.get()].kill();

    return DelistFileResult::SUCCESS;
}

manager::FileManager::GetFileResult manager::FileManager::get_file(manager_types::FileId file_id) {
    if (file_id.get() >= files.size())
        return std::nullopt;

    std::size_t numeric_file_id = file_id.get();

    if (!files[numeric_file_id].is_alive())
        return std::nullopt;

    return files[numeric_file_id].get_file();
}

manager::FileManager::GetConstFileResult manager::FileManager::get_file(manager_types::FileId file_id) const {
    if (file_id.get() >= files.size())
        return std::nullopt;

    std::size_t numeric_file_id = file_id.get();

    if (!files[numeric_file_id].is_alive())
        return std::nullopt;

    return std::cref(files[numeric_file_id].get_file());
}

manager::FileManager::FindFileResult manager::FileManager::find_file(std::string path) const {
    for (std::size_t file_id = 0; file_id < files.size(); file_id++) {
        if (files[file_id].is_alive() && files[file_id].get_file().path == path)
            return manager_types::FileId{file_id};
    }

    return std::nullopt;
}

std::vector<manager_types::FileId> manager::FileManager::get_valid_files() const {
    std::vector<manager_types::FileId> valid_files;

    for (std::size_t file_id = 0; const FileEntry& file_entry : files) {
        if (file_entry.is_alive())
            valid_files.push_back(manager_types::FileId{file_id});
    }

    return valid_files;
}

bool manager::FileManager::has_errors() const {
    return std::find_if(files.begin(), files.end(), [](const FileEntry& file_entry) -> bool {
        return file_entry.is_alive() && file_entry.get_file().logger.has_errors();
    }) != files.end();
}

// call this once the seed file_id 
void manager::FileRefresher::recurse_mark_dirty(manager_types::FileId start) {
    std::vector<manager_types::FileId> file_stack;

    if (!file_manager.get_file(start).has_value())
        util::panic("The central dependent invalidator attempted to invalidate a nonexistent file.");

    file_stack.push_back(start);

    while (!file_stack.empty()) {
        manager_types::FileId file_id = file_stack.back();
        FileManager::GetFileResult get_file_result = file_manager.get_file(file_id).value();

        CompilationFile& file = get_file_result.value(); // checked later in the loop and refresh_files()

        if (file.dependency_data.is_dirty) {
            file_stack.pop_back();
            continue;
        }

        file.dependency_data.is_dirty = true;
        dirty_files.push_back(file_id);

        for (manager_types::FileId dependent_id : file.dependency_data.dependent_ids) {
            FileManager::GetFileResult get_dependent_file_result = file_manager.get_file(dependent_id);

            if (!get_dependent_file_result.has_value())
                util::panic("The central dependent invalidator caught a listed dependent file that does not exist.");
            
            CompilationFile& dependent_file = get_dependent_file_result.value();
            dependent_file.dependency_data.dirty_dependency_ids.push_back(file_id);

            file_stack.push_back(dependent_id);
        }
    }
}

manager::FileManager::DelistFileResult manager::FileRefresher::delist_file(manager_types::FileId file_id) {
    FileManager::GetFileResult get_file_result = file_manager.get_file(file_id);

    if (!get_file_result.has_value())
        return FileManager::DelistFileResult::FILE_DOESNT_EXIST;

    for (manager_types::FileId dependent_id : get_file_result.value().get().dependency_data.dependent_ids) {
        recurse_mark_dirty(dependent_id);
    }

    return file_manager.delist_file(file_id); // will check if file exists but we did here too so its ok
}

void manager::FileRefresher::refresh_files() {
    for (manager_types::FileId file_id : file_manager.get_valid_files()) {
        FileManager::GetFileResult get_file_result = file_manager.get_file(file_id);
        CompilationFile& file = get_file_result.value();

        // first check if the file even "exists"
        if (!std::filesystem::exists(file.path)) {
            file_manager.delist_file(file_id);
            continue;
        }

        // potentially remove files with no connections to others after not being referenced for multiple recompilations

        if (file.refresh_source_code()) {
            recurse_mark_dirty(file_id);
            continue;
        }
    }
}

void manager::CompilationEngine::compile() {
    FileManager::FindFileResult get_file_result = file_manager.find_file(engine_context.start_path);

    if (!get_file_result.has_value()) {
        std::cerr << "Failed to find the project's root path! Ensure there there is a path for \"" << engine_context.start_path << "\"\n";
        return;
    }

    manager_types::FileId file_id = get_file_result.value();
    CompilationFile& file = file_manager.get_file(file_id).value();

    if (file.state == FileState::COMPILE_READY) {
        run_frontend(file_id);
        return;
    }

    // otherwise, run recompilation

    file_refresher.refresh_files();
    recompile_dirty_files();
}

void manager::CompilationEngine::recompile_dirty_files() {
    file_refresher.dirty_files.erase(std::remove_if(file_refresher.dirty_files.begin(), file_refresher.dirty_files.end(), [&](manager_types::FileId dirty_file_id) {
        const CompilationFile& dirty_file = file_manager.get_file(dirty_file_id).value(); // checked earlier in recurse_mark_dirty
        const bool is_file_dependent = dirty_file.dependency_data.dirty_dependency_ids.size() > 0;

        return is_file_dependent;
    }));

    while (file_refresher.dirty_files.size() > 0) {
        manager_types::FileId file_id = file_refresher.dirty_files.back();
        CompilationFile& file = file_manager.get_file(file_id).value();

        // grab list of dependents before it gets deleted
        std::vector<manager_types::FileId> dependent_ids = file.dependency_data.dependent_ids;

        // ensure something really wrong isnt happening
        util::panic_assert(file.dependency_data.dirty_dependency_ids.size() == 0, "");

        // reset file state
        file.compiler_output_data.clear();
        file.state = FileState::COMPILE_READY;

        // yeah, this is happening. this function directly handles dependency data, and therefore can safely modify it.
        file.dependency_data.dependent_ids.clear();

        // make file "clean" again
        file.dependency_data.is_dirty = false;

        // state machine prevents reprocessing due to states
        run_frontend(file_id);

        file_refresher.dirty_files.erase(file_refresher.dirty_files.end() - 1);

        // a dependent file of a dirty file is always going to be dirty
        // enforced by recurse_mark_dirty
        for (manager_types::FileId dependent_file_id : dependent_ids) {
            CompilationFile& dependent_file = file_manager.get_file(dependent_file_id).value();
            dependent_file.dependency_data.remove_dirty_dependency(file_id);

            // push front to maintain proper compilation ordering of dependencies (root -> dependent -> dependent's dependent)
            file_refresher.dirty_files.push_front(dependent_file_id);
        }
    }
}

void manager::CompilationEngine::run_frontend(manager_types::FileId target_file_id) {
    std::vector<manager_types::FileId> file_stack;
    file_stack.emplace_back(target_file_id);

    while (!file_stack.empty()) {
        manager_types::FileId file_id = file_stack.back();
        CompilationFile& file = file_manager.get_file(file_id).value();

        switch (file.state) {
            case FileState::COMPILE_READY:
                parse_file(*this, file_id, file_stack);
                file.state = FileState::SCANNED;
                break;

            case FileState::SCANNED:
                analyze_file(*this, file_id);
                file.state = FileState::ANALYZED;
                break;

            case FileState::ANALYZED:       
                file.state = FileState::IR_GENERATED;
                break;

            case FileState::IR_GENERATED:
                file.state = FileState::DONE;
                break;

            case FileState::DONE:
                file_stack.pop_back();
                std::cout << "Finished frontend for [" << file.path << "].\n";
                std::cout << file.logger.to_string();
                break;
        }
    }
}