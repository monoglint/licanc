#include "frontend/manager.hh"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "frontend/scan/lexer.hh"
#include "frontend/scan/parser.hh"
#include "frontend/sema/semantic_analyzer.hh"

#include "util/ansi_format.hh"
#include "util/panic.hh"

namespace {
    enum class t_open_file_error {
        COULDNT_OPEN_FILE,
        PATH_INVALID
    };

    using t_open_file_result = std::expected<std::string, t_open_file_error>;
    t_open_file_result open_file(std::string file_path) {
        if (!std::filesystem::exists(file_path))
            return std::unexpected(t_open_file_error::PATH_INVALID);

        std::ifstream input_file(file_path, std::ios::binary);

        if (!input_file || !input_file.is_open())
            return std::unexpected(t_open_file_error::COULDNT_OPEN_FILE);

        return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());    
    }
}

namespace frontend::manager {
namespace {
    // used by handle_file_imports to mark the current file as an added or existing file's dependency
    // returns the file_id of the new dependency of the given file
    /*
    
    file_id -> file_id of the file that is now a registered dependent of the file who's path is "absolute_file_path"
    
    */
    t_file_id register_file_dependency(t_frontend_unit& unit, t_file_id file_id, const std::string& absolute_file_path) {
        t_frontend_files::t_add_file_result add_file_result = unit.files.add_file(absolute_file_path);
        
        if (add_file_result.has_value()) {
            t_file_id new_file_id = add_file_result.value();
            t_frontend_file& new_file = unit.files.get_file(new_file_id).value();

            new_file.dependency_data.dependent_ids.push_back(file_id);

            return new_file_id;
        }

        else if (add_file_result.error() == t_frontend_files::t_add_file_error::FILE_ALREADY_EXISTS) {
            t_file_id found_file_id = unit.files.find_file(absolute_file_path).value();
            t_frontend_file& found_file = unit.files.get_file(found_file_id).value();
            
            found_file.dependency_data.dependent_ids.push_back(file_id);

            return found_file_id;
        }
        
        util::panic("An unreachable condition was reached by the file dependency registerer. Whether or not 'absolute_file_path' is valid was checked by the file dependency register's only caller.");
    }

    bool detect_file_interdependency(const t_frontend_unit& unit, t_file_id current_file_id, std::vector<t_file_id>& file_stack, const std::string& absolute_file_path) {
        return std::find_if(file_stack.begin(), file_stack.end(), [&](t_file_id frame_file_id) -> bool {
            if (current_file_id == frame_file_id)
                return false;

            t_frontend_files::t_get_const_file_result frame_get_file_result = unit.files.get_file(frame_file_id);
            const t_frontend_file& frame_file = frame_get_file_result.value();

            return frame_file.path == absolute_file_path;

        }) != file_stack.end();
    }
    /*

    lazily loading in all of the files we need should occur after parsing because we have just enough information to know what files we need to use.

    the semantic analyzer will take care of linking references between files together, but actually loading included files can occur
    after parsing. this lessens complexity
    
    */
    void handle_file_imports(t_frontend_unit& unit, t_file_id current_file_id, t_frontend_file& file, std::vector<t_file_id>& file_stack) {
        // check if there is an include item anywhere in the ast.
        for (scan::ast::t_import_decl* import_decl_node : file.ast.import_nodes) {
            if (!import_decl_node->is_path_valid)
                continue;

            t_compile_time_data::t_string_literal_pool::t_get_result get_string_result = 
                unit.compile_time_data.string_literal_pool.get(import_decl_node->absolute_file_path->string_literal_id);

            const std::string& absolute_file_path = get_string_result.value();

            bool interdependency_found = detect_file_interdependency(unit, current_file_id, file_stack, absolute_file_path);
            if (interdependency_found) {
                file.logger.add_error(import_decl_node->span, std::string("Including \\" + absolute_file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                continue;
            }

            t_file_id dependency_file_id = register_file_dependency(unit, current_file_id, absolute_file_path);

            import_decl_node->resolved_file_id = dependency_file_id;
            file_stack.emplace_back(dependency_file_id);
        }
    }

    void parse_file(t_frontend_unit& unit, t_file_id file_id, std::vector<t_file_id>& file_stack) {
        t_frontend_file& file = unit.files.get_file(file_id).value();

        // not implemented yet
        scan::lexer::lex(scan::lexer::t_lexer_context{});
        scan::parser::parse(scan::parser::t_parser_context{
            .ast = file.ast
        });

        handle_file_imports(unit, file_id, file, file_stack);
    }

    void analyze_file(t_frontend_unit& unit, t_file_id file_id) {
        sema::semantic_analyzer::analyze(unit, file_id);
    }
}
}

std::string frontend::manager::t_log::to_string(bool format) const {
    std::stringstream buffer;

    if (format) {
        buffer << util::ansi_format::LIGHT_GRAY << span.start.to_string() << ":\n" << util::ansi_format::RESET;

        switch (log_type) {
            case t_log_type::MESSAGE:
                buffer << util::ansi_format::CYAN;
                break;
            case t_log_type::WARNING:
                buffer << util::ansi_format::YELLOW << util::ansi_format::BOLD << "Warning: ";
                break;
            case t_log_type::ERROR:
                buffer << util::ansi_format::RED << util::ansi_format::BOLD << "Error: ";
                break;
        }
    }
    else {
        buffer << span.start.to_string() << ":\n";
        switch (log_type) {
            case t_log_type::MESSAGE:
                break;
            case t_log_type::WARNING:
                buffer << "Warning: ";
                break;
            case t_log_type::ERROR:
                buffer << "Error: ";
                break;
        }
    }
        
    buffer << message;
    
    if (format)
        buffer << util::ansi_format::RESET;

    return buffer.str();
}

std::string frontend::manager::t_logger::to_string(bool format) const {
    std::stringstream buffer;

    buffer << "Logs:\n";
    for (const t_log& log : logs) {
        buffer << log.to_string(format) << "\n\n";
    }

    return buffer.str();
}

void frontend::manager::t_file_dependency_data::remove_dirty_dependency(t_file_id now_clean_dependency_id) {
    std::erase_if(dirty_dependency_ids, [&](const t_file_id dirty_dependency_id) -> bool {
        return dirty_dependency_id == now_clean_dependency_id;
    });
}

void frontend::manager::t_frontend_file::clear_frontend_pass_data() {
    logger.clear();
    tokens.clear();
    ast.clear();
    sym_table.clear();
}

bool frontend::manager::t_frontend_file::refresh_source_code() {
    t_open_file_result open_file_result = open_file(path);

    if (!open_file_result.has_value()) {
        std::cerr << "Licanc was unable to open \"" 
            << path 
            << "\" to compare to a cached version. The compiler will not register any changes made to the file since the last compilation.\n";

        return false;
    }
    
    if (open_file_result.value() == source_code)
        return false;
    
    source_code = open_file_result.value();

    return true;
}

bool frontend::manager::t_logger::has_errors() const {
    return std::find_if(logs.begin(), logs.end(), [](const t_log& log) -> bool {
        return log.log_type == t_log_type::ERROR;
    }) != logs.end();
}

void frontend::manager::t_frontend_unit::run_compiler_state_machine(t_file_id target_file_id) {
        std::vector<manager::t_file_id> file_stack;
    file_stack.emplace_back(target_file_id);

    while (!file_stack.empty()) {
        manager::t_file_id file_id = file_stack.back();

        manager::t_frontend_files::t_get_file_result get_file_result = files.get_file(file_id);

        util::panic_assert(get_file_result.has_value(), "State machine encountered a nonexistent file in the stack.");

        t_frontend_file& file = get_file_result.value();

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

frontend::manager::t_frontend_files::t_add_file_result frontend::manager::t_frontend_files::add_file(std::string path) {
    const bool file_exists = std::find_if(files.begin(), files.end(), [&path](const std::optional<t_frontend_file>& file) -> bool {
        return file.has_value() && file.value().path == path;
    }) != files.end();
    
    if (file_exists)
        return std::unexpected(t_add_file_error::FILE_ALREADY_EXISTS);
    
    t_open_file_result open_file_result = open_file(path);

    if (!open_file_result.has_value()) {
        switch (open_file_result.error()) {
            case t_open_file_error::COULDNT_OPEN_FILE:
                return std::unexpected(t_add_file_error::COULDNT_OPEN_FILE);
            case t_open_file_error::PATH_INVALID:
                return std::unexpected(t_add_file_error::PATH_INVALID);
        }
    }

    files.emplace_back(std::filesystem::absolute(path).string(), open_file_result.value());

    return t_file_id{files.size() - 1};
}

frontend::manager::t_frontend_files::t_delist_file_result frontend::manager::t_frontend_files::delist_file(t_file_id file_id) {
    if (file_id.get() >= files.size())
        return t_delist_file_result::FILE_DOESNT_EXIST;

    // intentionally not throwing an error for if the file was already delisted because i don't really know why i should
    
    // assignment to std::nullopt should free that space
    files[file_id.get()] = std::nullopt;

    return t_delist_file_result::SUCCESS;
}

frontend::manager::t_frontend_files::t_get_file_result frontend::manager::t_frontend_files::get_file(t_file_id file_id) {
    if (file_id.get() >= files.size())
        return std::nullopt;

    std::size_t numeric_file_id = file_id.get();

    if (!files[numeric_file_id].has_value())
        return std::nullopt;

    return files[numeric_file_id].value();
}

frontend::manager::t_frontend_files::t_get_const_file_result frontend::manager::t_frontend_files::get_file(t_file_id file_id) const {
    if (file_id.get() >= files.size())
        return std::nullopt;

    std::size_t numeric_file_id = file_id.get();

    if (!files[numeric_file_id].has_value())
        return std::nullopt;

    return std::cref(files[numeric_file_id].value());
}

frontend::manager::t_frontend_files::t_find_file_result frontend::manager::t_frontend_files::find_file(std::string path) const {
    for (std::size_t file_id = 0; file_id < files.size(); file_id++) {
        if (files[file_id].has_value() && files[file_id].value().path == path)
            return t_file_id{file_id};
    }

    return std::nullopt;
}

std::vector<frontend::manager::t_file_id> frontend::manager::t_frontend_files::get_valid_files() const {
    std::vector<t_file_id> valid_files;

    for (std::size_t file_id = 0; const t_file_entry& file_entry : files) {
        if (file_entry.has_value())
            valid_files.push_back(t_file_id{file_id});
    }

    return valid_files;
}

bool frontend::manager::t_frontend_files::has_errors() const {
    return std::find_if(files.begin(), files.end(), [](const std::optional<t_frontend_file>& file) -> bool {
        return file.has_value() && file.value().logger.has_errors();
    }) != files.end();
}

// call this once the seed file_id 
void frontend::manager::t_frontend_files::recurse_mark_dirty(t_file_id start) {
    std::vector<t_file_id> file_stack;

    if (!get_file(start).has_value())
        util::panic("The central dependent invalidator attempted to invalidate a nonexistent file.");

    file_stack.push_back(start);

    while (!file_stack.empty()) {
        t_file_id file_id = file_stack.back();
        t_frontend_files::t_get_file_result get_file_result = get_file(file_id).value();
        
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

// -> namespace bound

frontend::manager::t_frontend_unit::t_frontend_unit(t_frontend_config _config) 
    : config(std::move(_config)) 
{
    t_frontend_files::t_add_file_result add_file_result = files.add_file(config.start_path);

    util::panic_assert(add_file_result.has_value(), "Failed to create root file for the primary compilation unit.");
}

/*

this function should wipe the file from the compier, make its id available, and make all of its dependents recompile

*/
frontend::manager::t_frontend_unit::t_delist_file_result frontend::manager::t_frontend_unit::delist_file(t_file_id file_id) {
    t_frontend_files::t_get_file_result get_file_result = files.get_file(file_id);

    if (!get_file_result.has_value())
        return t_delist_file_result::FILE_NOT_LISTED;

    std::vector<t_file_id> file_dependent_ids = get_file_result.value().get().dependency_data.dependent_ids;

    // this is gonna turn that file into garbage, don't bother even getting file by varaible before this
    files.delist_file(file_id);

    for (t_file_id dependent_id : file_dependent_ids) {
        files.recurse_mark_dirty(dependent_id);
    }
}

void frontend::manager::t_frontend_unit::compile() {
    t_frontend_files::t_find_file_result find_file_result = files.find_file(config.start_path);
    if (!find_file_result.has_value()) {
        std::cerr << "Could not locate a root file to begin compilation from. Please try again.\n";
        return;
    }
    
    t_file_id root_file_id = find_file_result.value();
    t_frontend_file& root_file = files.get_file(root_file_id).value();

    if (root_file.state == t_file_state::SCAN_READY) {
        run_compiler_state_machine(root_file_id);
        return;
    }

    refresh_files();
    recompile_dirty_files();
}

void frontend::manager::t_frontend_unit::refresh_files() {
    for (t_file_id file_id : files.get_valid_files()) {
        t_frontend_files::t_get_file_result get_file_result = files.get_file(file_id);
        t_frontend_file& file = get_file_result.value();

        // first check if the file even "exists"
        if (!std::filesystem::exists(file.path)) {
            delist_file(file_id);
            continue;
        }

        // potentially remove files with no connections to others after not being referenced for multiple recompilations

        if (file.refresh_source_code()) {
            files.recurse_mark_dirty(file_id);
            continue;
        }
    }
}

void frontend::manager::t_frontend_unit::recompile_dirty_files() {
    files.dirty_files.erase(std::remove_if(files.dirty_files.begin(), files.dirty_files.end(), [&](t_file_id dirty_file_id) {
        const t_frontend_file& dirty_file = files.get_file(dirty_file_id).value(); // checked earlier in recurse_mark_dirty
        const bool is_file_dependent = dirty_file.dependency_data.dirty_dependency_ids.size() > 0;

        return is_file_dependent;
    }));

    while (files.dirty_files.size() > 0) {
        t_file_id file_id = files.dirty_files.back();
        t_frontend_file& file = files.get_file(file_id).value();

        // grab list of dependents before it gets deleted
        std::vector<t_file_id> dependent_ids = file.dependency_data.dependent_ids;

        // ensure something really wrong isnt happening
        util::panic_assert(file.dependency_data.dirty_dependency_ids.size() == 0, "");

        // reset file state
        file.clear_frontend_pass_data();
        file.state = t_file_state::SCAN_READY;

        // yeah, this is happening. this function directly handles dependency data, and therefore can safely modify it.
        file.dependency_data.dependent_ids.clear();

        // make file "clean" again
        file.dependency_data.is_dirty = false;

        // state machine prevents reprocessing due to states
        run_compiler_state_machine(file_id);

        files.dirty_files.erase(files.dirty_files.end() - 1);

        // a dependent file of a dirty file is always going to be dirty
        // enforced by recurse_mark_dirty
        for (t_file_id dependent_file_id : dependent_ids) {
            t_frontend_file& dependent_file = files.get_file(dependent_file_id).value();
            dependent_file.dependency_data.remove_dirty_dependency(file_id);

            // push front to maintain proper compilation ordering of dependencies (root -> dependent -> dependent's dependent)
            files.dirty_files.push_front(dependent_file_id);
        }
    }
}

std::string frontend::manager::standardize_import_path(std::string project_path, std::string path) {
    /*
        ways a path can be formatted:
        
        1. with no relativity
        import "C:/users/foobar/lican/libraries/monolib/arena.li"
        
        2. relative to project
        import "lib/gl/render.li"

        3. relative to a custom set import directoy
        [self explanatory]
    */


}