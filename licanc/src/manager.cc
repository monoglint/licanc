#include "frontend/manager.hh"

#include <iostream>
#include <iterator>

#include "frontend/scan/lexer.hh"
#include "frontend/scan/parser.hh"
#include "frontend/sema/semantic_analyzer.hh"

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