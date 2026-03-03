#include "frontend/manager.hh"

#include <iostream>
#include <iterator>

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