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
    manager::t_file_id register_file_dependency(manager::t_compilation_engine& engine, manager::t_file_id file_id, const std::string& absolute_file_path) {
        manager::t_file_manager::t_add_file_result add_file_result = engine.file_manager.add_file(absolute_file_path);
        
        if (add_file_result.has_value()) {
            manager::t_file_id new_file_id = add_file_result.value();
            manager::t_compilation_file& new_file = engine.file_manager.get_file(new_file_id).value();

            new_file.dependency_data.dependent_ids.push_back(file_id);

            return new_file_id;
        }

        else if (add_file_result.error() == manager::t_file_manager::t_add_file_error::FILE_ALREADY_EXISTS) {
            manager::t_file_id found_file_id = engine.file_manager.find_file(absolute_file_path).value();
            manager::t_compilation_file& found_file = engine.file_manager.get_file(found_file_id).value();
            
            found_file.dependency_data.dependent_ids.push_back(file_id);

            return found_file_id;
        }
        
        util::panic("An unreachable condition was reached by the file dependency registerer. Whether or not 'absolute_file_path' is valid was checked by the file dependency register's only caller.");
    }

    bool detect_file_interdependency(const manager::t_compilation_engine& engine, manager::t_file_id current_file_id, std::vector<manager::t_file_id>& file_stack, const std::string& absolute_file_path) {
        return std::find_if(file_stack.begin(), file_stack.end(), [&](manager::t_file_id frame_file_id) -> bool {
            if (current_file_id == frame_file_id)
                return false;

            manager::t_file_manager::t_get_const_file_result frame_get_file_result = engine.file_manager.get_file(frame_file_id);
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

            bool interdependency_found = detect_file_interdependency(engine, current_file_id, file_stack, absolute_file_path);
            if (interdependency_found) {
                file.logger.add_error(import_decl_node->span, std::string("Including \\" + absolute_file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                continue;
            }

            manager::t_file_id dependency_file_id = register_file_dependency(engine, current_file_id, absolute_file_path);

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
void manager::t_scheduler::recurse_mark_dirty(manager::t_file_id start) {
    std::vector<manager::t_file_id> file_stack;

    if (!file_manager.get_file(start).has_value())
        util::panic("The central dependent invalidator attempted to invalidate a nonexistent file.");

    file_stack.push_back(start);

    while (!file_stack.empty()) {
        t_file_id file_id = file_stack.back();
        t_file_manager::t_get_file_result get_file_result = get_file(file_id).value();

        t_frontend_file& file = get_file_result.value(); // checked later in the loop and refresh_files()

        if (file.dependency_data.is_dirty) {
            file_stack.pop_back();
            continue;
        }

        file.dependency_data.is_dirty = true;
        dirty_files.push_back(file_id);

        for (t_file_id dependent_id : file.dependency_data.dependent_ids) {
            t_frontend_files::t_get_file_result get_dependent_file_result = get_file(dependent_id);

            if (!get_dependent_file_result.has_value())
                util::panic("The central dependent invalidator caught a listed dependent file that does not exist.");
            
            t_frontend_file& dependent_file = get_dependent_file_result.value();
            dependent_file.dependency_data.dirty_dependency_ids.push_back(file_id);

            file_stack.push_back(dependent_id);
        }
    }
}