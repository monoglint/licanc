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
    std::optional<std::string> open_file(std::string file_path) {
        std::ifstream input_file(file_path, std::ios::binary);

        if (!input_file || !input_file.is_open())
            return std::nullopt;

        return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());    
    }
}

// everything in this namespace could be a method of t_compilation_file
// they are not methods to maintain encapsulation, and so manager.hh doesn't need to #include <stack>
namespace {
    /*

    lazily loading in all of the files we need should occur after parsing because we have just enough information to know what files we need to use.

    the semantic analyzer will take care of linking references between files together, but actually loading included files can occur
    after parsing. this lessens complexity
    
    */
    void handle_file_imports(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id, frontend::manager::t_compilation_file& file, std::vector<frontend::manager::t_file_id>& file_stack) {
        // check if there is an include item anywhere in the ast.
        for (frontend::scan::ast::t_import_decl* import_decl_node : file.ast.import_nodes) {
            frontend::manager::t_compile_time_data::t_string_literal_pool::t_get_result get_string_result = 
                unit.compile_time_data.string_literal_pool.get(import_decl_node->absolute_file_path->string_literal_id);

            const std::string& absolute_file_path = get_string_result.value(); // explicit bypass

            // we're calling the file frame_file since we're iterating through indexes of a stack
            bool interdependency_found = std::find_if(file_stack.begin(), file_stack.end(), [&absolute_file_path, &unit](frontend::manager::t_file_id frame_file_id) -> bool {
                frontend::manager::t_compilation_files::t_get_file_result frame_get_file_result = unit.files.get_file(frame_file_id);
                frontend::manager::t_compilation_file& frame_file = frame_get_file_result.value();

                return frame_file.path == absolute_file_path;

            }) != file_stack.end();

            if (interdependency_found) {
                file.logger.add_error(import_decl_node->span, std::string("Including \\" + absolute_file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                return;
            }

            frontend::manager::t_compilation_files::t_add_file_result add_file_result = unit.files.add_file(absolute_file_path);
            
            if (add_file_result.has_value()) {
                file_stack.emplace_back(add_file_result.value());
                import_decl_node->resolved_file_id = add_file_result.value();

                unit.files.get_file(add_file_result.value()).value().get().moocher_ids.push_back(file_id);
            }
            else if (add_file_result.error() == frontend::manager::t_compilation_files::t_add_file_error::FILE_ALREADY_EXISTS) {
                frontend::manager::t_file_id found_file_id = unit.files.find_file(absolute_file_path).value(); // explicit bypass
                import_decl_node->resolved_file_id = found_file_id;

                frontend::manager::t_compilation_file& found_file = unit.files.get_file(found_file_id).value(); //explicit bypass
                if (found_file.state == frontend::manager::t_file_state::DONE)
                    found_file.moocher_ids.push_back(file_id);
            }
                
                // note: if the existing file is RECOMPILE_READY, then mark it as scan ready and put it back in the file stack.

            else if (add_file_result.error() == frontend::manager::t_compilation_files::t_add_file_error::PATH_INVALID)
                file.logger.add_error(import_decl_node->span, "The file \\" + absolute_file_path + "\" does not exist.");
        }
    }

    void parse_file(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id, std::vector<frontend::manager::t_file_id>& file_stack) {
        frontend::manager::t_compilation_file& file = unit.files.get_file(file_id).value();

        // not implemented yet
        frontend::scan::lexer::lex(frontend::scan::lexer::t_lexer_context{});
        frontend::scan::parser::parse(frontend::scan::parser::t_parser_context{});

        handle_file_imports(unit, file_id, file, file_stack);
    }

    void analyze_file(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id) {
        frontend::manager::t_compilation_files::t_get_file_result get_file_result = unit.files.get_file(file_id);

        // GET_FILE_RESULT IS CONFIRMED TO EXIST BY THE STACK LOOP THIS FUNCITON IS CALLED IN

        frontend::sema::semantic_analyzer::analyze(unit, file_id);
    }

    /*
    
    if a file is about to be recompiled, moochers need to be cleared and prepared for recompilation
    
    tbh this function is a tiny bit hacky, because it relies on the state machine and the state maachine relies on it
    to get the job done, but it works and it is

    even though moochers are registered after they are done being scanned, they will ALWAYS BE DONE by the time they are recognized as a moocher.
    */
    void invalidate_file_moochers(frontend::manager::t_compilation_unit& unit, frontend::manager::t_compilation_file& file, std::vector<frontend::manager::t_file_id>& file_stack) {
        for (frontend::manager::t_file_id moocher_id : file.moocher_ids) {
            file_stack.push_back(moocher_id);
        }
    }
}

std::string frontend::manager::t_log::to_string(bool format) const {
    std::stringstream buffer;

    if (format) {
        buffer 
            << util::ansi_format::LIGHT_GRAY
            << span.start.to_string()
            << ":\n"
            << util::ansi_format::RESET;

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

bool frontend::manager::t_logger::has_errors() const {
    return std::find_if(logs.begin(), logs.end(), [](const t_log& log) -> bool {
        return log.log_type == t_log_type::ERROR;
    }) != logs.end();
}

// base function for parse_file(), analyze_file(), and others
void frontend::manager::t_compilation_unit::compile(frontend::manager::t_file_id target_file_id) {
    std::vector<manager::t_file_id> file_stack;
    file_stack.emplace_back(target_file_id);

    while (!file_stack.empty()) {
        manager::t_file_id file_id = file_stack.back();

        manager::t_compilation_files::t_get_file_result get_file_result = files.get_file(file_id);
        // sanity recheck
        if (!get_file_result.has_value()) {
            std::cerr << "For some reason, there was a non existent file in the file stack. Might wanna check that out.\n";
            file_stack.pop_back();
            continue;
        }

        t_compilation_file& file = get_file_result.value();

        switch (file.state) {
            case t_file_state::SCAN_READY:
                parse_file(*this, file_id, file_stack);
                file.state = t_file_state::SEMA_READY;
                break;

            case t_file_state::SEMA_READY:
                analyze_file(*this, file_id);
                file.state = t_file_state::DONE;

                file_stack.pop_back();
                std::cout << "Finished frontend for [" << file.path << "].\n"; 
                std::cout << file.logger.to_string();
                break;

            // only occurs when we want to recompile
            case t_file_state::DONE:
                invalidate_file_moochers(*this, file, file_stack);
                clear_file(file_id);
                file.state = t_file_state::SCAN_READY;
                break;
        }
    }
}

// -> t_compilation_unit

frontend::manager::t_compilation_files::t_add_file_result frontend::manager::t_compilation_files::add_file(std::string path) {
    bool file_exists = std::find_if(files.begin(), files.end(), [&path](t_compilation_file& file) -> bool {
        return file.path == path;
    }) != files.end();
    
    if (file_exists)
        return std::unexpected(t_add_file_error::FILE_ALREADY_EXISTS);
    
    std::optional<std::string> open_file_result = open_file(path);

    if (!open_file_result.has_value())
        return std::unexpected(t_add_file_error::PATH_INVALID);

    files.emplace_back(path, open_file_result.value());
    return t_file_id{files.size() - 1};
}

frontend::manager::t_compilation_files::t_get_file_result frontend::manager::t_compilation_files::get_file(t_file_id file_id) {
    if (static_cast<std::size_t>(file_id) >= files.size())
        return std::nullopt;

    return files.at(static_cast<std::size_t>(file_id));
}

frontend::manager::t_compilation_files::t_get_const_file_result frontend::manager::t_compilation_files::get_file(t_file_id file_id) const {
    if (static_cast<std::size_t>(file_id) >= files.size())
        return std::nullopt;

    return std::cref(files.at(static_cast<std::size_t>(file_id)));
}

frontend::manager::t_compilation_files::t_find_file_result frontend::manager::t_compilation_files::find_file(std::string path) const {
    for (std::size_t i = 0; i < files.size(); i++) {
        if (files[i].path == path)
            return t_file_id{i};
    }

    return std::nullopt;
}

bool frontend::manager::t_compilation_files::has_errors() const {
    return std::find_if(files.begin(), files.end(), [](const t_compilation_file& file) -> bool {
        return file.logger.has_errors();
    }) != files.end();
}

// -> namespace bound

frontend::manager::t_compilation_unit::t_compilation_unit(t_frontend_config _config) 
    : config(std::move(_config)) {
    std::string start_path = config.project_path + '/' + config.start_subpath;
    
    t_compilation_files::t_add_file_result add_file_result = files.add_file(start_path);

    util::panic_assert(add_file_result.has_value(), "Failed to create root file for the primary compilation unit.");
}

void frontend::manager::t_compilation_unit::compile() {
    t_compilation_files::t_find_file_result find_file_result = files.find_file(config.project_path + '/' + config.start_subpath);
    
    util::panic_assert(find_file_result.has_value(), "Could not locate a root file to begin compilation from.");

    compile(find_file_result.value());
}

void frontend::manager::t_compilation_unit::clear_file(t_file_id file_id) {
    
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