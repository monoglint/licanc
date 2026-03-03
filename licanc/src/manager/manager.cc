#include "manager/manager.hh"

#include <fstream>
#include <filesystem>
#include <expected>
#include <iostream>

#include "util/panic.hh"

#include "frontend/scan/lexer.hh"
#include "frontend/scan/parser.hh"
#include "frontend/sema/semantic_analyzer.hh"

namespace {
    enum class t_quick_read_file_error {
        COULDNT_OPEN_FILE,
        PATH_INVALID
    };

    using t_quick_read_file_result = std::expected<std::string, t_quick_read_file_error>;
    t_quick_read_file_result quick_read_file(std::string file_path) {
        if (!std::filesystem::exists(file_path))
            return std::unexpected(t_quick_read_file_error::PATH_INVALID);

        std::ifstream input_file(file_path);

        if (!input_file || !input_file.is_open())
            return std::unexpected(t_quick_read_file_error::COULDNT_OPEN_FILE);

        return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());    
    }
}

namespace {
    // used by handle_file_imports to mark the current file as an added or existing file's dependency
    // returns the file_id of the new dependency of the given file
    /*
    
    file_id -> file_id of the file that is now a registered dependent of the file who's path is "absolute_file_path"
    
    */
    manager::t_file_id register_file_dependency(manager::t_file_manager& file_manager, manager::t_file_id file_id, const std::string& absolute_file_path) {
        manager::t_file_manager::t_add_file_result add_file_result = file_manager.add_file(absolute_file_path);
        
        if (add_file_result.has_value()) {
            manager::t_file_id new_file_id = add_file_result.value();
            manager::t_compilation_file& new_file = file_manager.get_file(new_file_id).value();

            new_file.dependency_data.dependent_ids.push_back(file_id);

            return new_file_id;
        }

        else if (add_file_result.error() == manager::t_file_manager::t_add_file_error::FILE_ALREADY_EXISTS) {
            manager::t_file_id found_file_id = file_manager.find_file(absolute_file_path).value();
            manager::t_compilation_file& found_file = file_manager.get_file(found_file_id).value();
            
            found_file.dependency_data.dependent_ids.push_back(file_id);

            return found_file_id;
        }
        
        util::panic("An unreachable condition was reached by the file dependency registerer. Whether or not 'absolute_file_path' is valid was checked by the file dependency register's only caller.");
    }

    bool detect_file_interdependency(const manager::t_file_manager& file_manager, manager::t_file_id current_file_id, std::vector<manager::t_file_id>& file_stack, const std::string& absolute_file_path) {
        return std::find_if(file_stack.begin(), file_stack.end(), [&](manager::t_file_id frame_file_id) -> bool {
            if (current_file_id == frame_file_id)
                return false;

            manager::t_file_manager::t_get_const_file_result frame_get_file_result = file_manager.get_file(frame_file_id);
            const manager::t_compilation_file& frame_file = frame_get_file_result.value();

            return frame_file.path == absolute_file_path;

        }) != file_stack.end();
    }
    /*

    lazily loading in all of the files we need should occur after parsing because we have just enough information to know what files we need to use.

    the semantic analyzer will take care of linking references between files together, but actually loading included files can occur
    after parsing. this lessens complexity
    
    */
    void handle_post_parse_file_imports(manager::t_compilation_engine& engine, manager::t_file_id current_file_id, manager::t_compilation_file& file, std::vector<manager::t_file_id>& file_stack) {
        // check if there is an include item anywhere in the ast.
        for (frontend::scan::ast::t_import_decl* import_decl_node : file.compiler_output_data.frontend.ast.import_nodes) {
            if (!import_decl_node->is_path_valid)
                continue;

            manager::t_session_pools::t_string_literal_pool::t_const_get_result get_string_result = 
                engine.session_pools.string_literal_pool.get(import_decl_node->absolute_file_path->string_literal_id);

            const std::string& absolute_file_path = get_string_result.value();

            bool interdependency_found = detect_file_interdependency(engine.file_manager, current_file_id, file_stack, absolute_file_path);
            if (interdependency_found) {
                file.logger.add_error(import_decl_node->span, std::string("Including \\" + absolute_file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                continue;
            }

            manager::t_file_id dependency_file_id = register_file_dependency(engine.file_manager, current_file_id, absolute_file_path);

            import_decl_node->resolved_file_id = dependency_file_id;
            file_stack.emplace_back(dependency_file_id);
        }
    }

    void parse_file(manager::t_compilation_engine& engine, manager::t_file_id file_id, std::vector<manager::t_file_id>& file_stack) {
        manager::t_compilation_file& file = engine.file_manager.get_file(file_id).value();

        // not implemented yet
        frontend::scan::lexer::lex(frontend::scan::lexer::t_lexer_context{});
        frontend::scan::parser::parse(frontend::scan::parser::t_parser_context{
            .ast = file.compiler_output_data.frontend.ast
        });

        handle_post_parse_file_imports(engine, file_id, file, file_stack);
    }

    void analyze_file(manager::t_compilation_engine& engine, manager::t_file_id file_id) {
        frontend::sema::semantic_analyzer::analyze(engine, file_id);
    }
}

void manager::t_file_dependency_data::remove_dirty_dependency(t_file_id now_clean_dependency_id) {
    std::erase_if(dirty_dependency_ids, [&](const t_file_id dirty_dependency_id) -> bool {
        return dirty_dependency_id == now_clean_dependency_id;
    });
}

bool manager::t_compilation_file::refresh_source_code() {
    t_quick_read_file_result quick_read_file_result = quick_read_file(path);

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

manager::t_file_manager::t_add_file_result manager::t_file_manager::add_file(std::string path) {
    const bool file_exists = std::find_if(files.begin(), files.end(), [&path](const std::optional<t_compilation_file>& file) -> bool {
        return file.has_value() && file.value().path == path;
    }) != files.end();
    
    if (file_exists)
        return std::unexpected(t_add_file_error::FILE_ALREADY_EXISTS);
    
    t_quick_read_file_result quick_read_file_result = quick_read_file(path);

    if (!quick_read_file_result.has_value()) {
        switch (quick_read_file_result.error()) {
            case t_quick_read_file_error::COULDNT_OPEN_FILE:
                return std::unexpected(t_add_file_error::COULDNT_OPEN_FILE);
            case t_quick_read_file_error::PATH_INVALID:
                return std::unexpected(t_add_file_error::PATH_INVALID);
        }
    }

    files.emplace_back(std::filesystem::absolute(path).string(), quick_read_file_result.value());

    return t_file_id{files.size() - 1};
}

manager::t_file_manager::t_delist_file_result manager::t_file_manager::delist_file(t_file_id file_id) {
    if (file_id.get() >= files.size())
        return t_delist_file_result::FILE_DOESNT_EXIST;

    // intentionally not throwing an error for if the file was already delisted because i don't really know why i should
    
    // assignment to std::nullopt should free that space
    files[file_id.get()] = std::nullopt;

    return t_delist_file_result::SUCCESS;
}

manager::t_file_manager::t_get_file_result manager::t_file_manager::get_file(t_file_id file_id) {
    if (file_id.get() >= files.size())
        return std::nullopt;

    std::size_t numeric_file_id = file_id.get();

    if (!files[numeric_file_id].has_value())
        return std::nullopt;

    return files[numeric_file_id].value();
}

manager::t_file_manager::t_get_const_file_result manager::t_file_manager::get_file(t_file_id file_id) const {
    if (file_id.get() >= files.size())
        return std::nullopt;

    std::size_t numeric_file_id = file_id.get();

    if (!files[numeric_file_id].has_value())
        return std::nullopt;

    return std::cref(files[numeric_file_id].value());
}

manager::t_file_manager::t_find_file_result manager::t_file_manager::find_file(std::string path) const {
    for (std::size_t file_id = 0; file_id < files.size(); file_id++) {
        if (files[file_id].has_value() && files[file_id].value().path == path)
            return t_file_id{file_id};
    }

    return std::nullopt;
}

std::vector<manager::t_file_id> manager::t_file_manager::get_valid_files() const {
    std::vector<t_file_id> valid_files;

    for (std::size_t file_id = 0; const t_file_entry& file_entry : files) {
        if (file_entry.has_value())
            valid_files.push_back(t_file_id{file_id});
    }

    return valid_files;
}

bool manager::t_file_manager::has_errors() const {
    return std::find_if(files.begin(), files.end(), [](const std::optional<t_compilation_file>& file) -> bool {
        return file.has_value() && file.value().logger.has_errors();
    }) != files.end();
}

// call this once the seed file_id 
void manager::t_file_refresher::recurse_mark_dirty(manager::t_file_id start) {
    std::vector<manager::t_file_id> file_stack;

    if (!file_manager.get_file(start).has_value())
        util::panic("The central dependent invalidator attempted to invalidate a nonexistent file.");

    file_stack.push_back(start);

    while (!file_stack.empty()) {
        t_file_id file_id = file_stack.back();
        t_file_manager::t_get_file_result get_file_result = file_manager.get_file(file_id).value();

        t_compilation_file& file = get_file_result.value(); // checked later in the loop and refresh_files()

        if (file.dependency_data.is_dirty) {
            file_stack.pop_back();
            continue;
        }

        file.dependency_data.is_dirty = true;
        dirty_files.push_back(file_id);

        for (t_file_id dependent_id : file.dependency_data.dependent_ids) {
            t_file_manager::t_get_file_result get_dependent_file_result = file_manager.get_file(dependent_id);

            if (!get_dependent_file_result.has_value())
                util::panic("The central dependent invalidator caught a listed dependent file that does not exist.");
            
            t_compilation_file& dependent_file = get_dependent_file_result.value();
            dependent_file.dependency_data.dirty_dependency_ids.push_back(file_id);

            file_stack.push_back(dependent_id);
        }
    }
}

manager::t_file_manager::t_delist_file_result manager::t_file_refresher::delist_file(t_file_id file_id) {
    t_file_manager::t_get_file_result get_file_result = file_manager.get_file(file_id);

    if (!get_file_result.has_value())
        return t_file_manager::t_delist_file_result::FILE_DOESNT_EXIST;

    for (t_file_id dependent_id : get_file_result.value().get().dependency_data.dependent_ids) {
        recurse_mark_dirty(dependent_id);
    }

    return file_manager.delist_file(file_id); // will check if file exists but we did here too so its ok
}

void manager::t_file_refresher::refresh_files() {
    for (t_file_id file_id : file_manager.get_valid_files()) {
        t_file_manager::t_get_file_result get_file_result = file_manager.get_file(file_id);
        t_compilation_file& file = get_file_result.value();

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

void manager::t_compilation_engine::compile() {
    t_file_manager::t_find_file_result get_file_result = file_manager.find_file(config_context.start_path);

    if (!get_file_result.has_value()) {
        std::cerr << "Failed to find the project's root path! Ensure there there is a path for \"" << config_context.start_path << "\"\n";
        return;
    }

    t_file_id file_id = get_file_result.value();
    t_compilation_file& file = file_manager.get_file(file_id).value();

    if (file.state == t_file_state::SCAN_READY) {
        compile_to_completion(file_id);
        return;
    }

    // otherwise, run recompilation

    file_refresher.refresh_files();
    recompile_dirty_files();
}

void manager::t_compilation_engine::recompile_dirty_files() {
    file_refresher.dirty_files.erase(std::remove_if(file_refresher.dirty_files.begin(), file_refresher.dirty_files.end(), [&](t_file_id dirty_file_id) {
        const t_compilation_file& dirty_file = file_manager.get_file(dirty_file_id).value(); // checked earlier in recurse_mark_dirty
        const bool is_file_dependent = dirty_file.dependency_data.dirty_dependency_ids.size() > 0;

        return is_file_dependent;
    }));

    while (file_refresher.dirty_files.size() > 0) {
        t_file_id file_id = file_refresher.dirty_files.back();
        t_compilation_file& file = file_manager.get_file(file_id).value();

        // grab list of dependents before it gets deleted
        std::vector<t_file_id> dependent_ids = file.dependency_data.dependent_ids;

        // ensure something really wrong isnt happening
        util::panic_assert(file.dependency_data.dirty_dependency_ids.size() == 0, "");

        // reset file state
        file.compiler_output_data.clear();
        file.state = t_file_state::SCAN_READY;

        // yeah, this is happening. this function directly handles dependency data, and therefore can safely modify it.
        file.dependency_data.dependent_ids.clear();

        // make file "clean" again
        file.dependency_data.is_dirty = false;

        // state machine prevents reprocessing due to states
        compile_to_completion(file_id);

        file_refresher.dirty_files.erase(file_refresher.dirty_files.end() - 1);

        // a dependent file of a dirty file is always going to be dirty
        // enforced by recurse_mark_dirty
        for (t_file_id dependent_file_id : dependent_ids) {
            t_compilation_file& dependent_file = file_manager.get_file(dependent_file_id).value();
            dependent_file.dependency_data.remove_dirty_dependency(file_id);

            // push front to maintain proper compilation ordering of dependencies (root -> dependent -> dependent's dependent)
            file_refresher.dirty_files.push_front(dependent_file_id);
        }
    }
}

void manager::t_compilation_engine::compile_to_completion(t_file_id target_file_id) {
    std::vector<manager::t_file_id> file_stack;
    file_stack.emplace_back(target_file_id);

    while (!file_stack.empty()) {
        manager::t_file_id file_id = file_stack.back();
        t_compilation_file& file = file_manager.get_file(file_id).value();

        switch (file.state) {
            case t_file_state::SCAN_READY:
                parse_file(*this, file_id, file_stack);
                file.state = t_file_state::SEMA_READY;
                break;

            case t_file_state::SEMA_READY:
                analyze_file(*this, file_id);
                file.state = t_file_state::DONE;
                break;

            case t_file_state::DONE:       
                file_stack.pop_back();
                std::cout << "Finished frontend for [" << file.path << "].\n"; 
                std::cout << file.logger.to_string();         
                break;
        }
    }
}