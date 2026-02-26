#include "frontend/manager.hh"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "frontend/scan/lexer.hh"
#include "frontend/scan/parser.hh"
#include "frontend/sema/semantic_analyzer.hh"

#include "util/ansi_format.hh"

namespace {
    std::optional<std::string> open_file(std::string file_path) {
        std::ifstream input_file(file_path);

        if (!input_file || !input_file.is_open())
            return std::nullopt;

        return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());    
    }
}

// everything in this namespace could be a method of t_compilation_file
// they are not methods to maintain encapsulation, and so manager.hh doesn't need to #include <stack>
namespace {
    void handle_file_imports(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id, frontend::manager::t_compilation_file& file, std::vector<frontend::manager::t_file_id>& file_stack) {
        using namespace frontend;

        // check if there is an include item anywhere in the ast.
        for (scan::ast::t_node_id import_node_id : file.import_node_ids) {
            
            scan::ast::t_node_id file_path_node_id = file.ast.get<scan::ast::t_import_item>(import_node_id).value().get().file_path; // bypass optional guard because import_node_ids only contains valid nodes ever
            scan::ast::t_string_literal& file_path_node = file.ast.get<scan::ast::t_string_literal>(file_path_node_id).value().get(); // ^^
            
            auto get_string_result = unit.compile_time_data.string_literal_pool.get(file_path_node.string_literal_id);

            if (!get_string_result.has_value()) {
                unit.logger.add_internal_error(file_id, file_path_node.span, "Failed to find an interned string in the string literal pool. If you see this error message, start praying for monoglint.");
                // attempt to import other files while at it. ultimately, the program is not going to build properly, but getting more errors is worth it.
                continue;
            }

            std::string& file_path = get_string_result.value();

            // we're calling the file frame_file since we're iterating through indexes of a stack
            bool interdependency_found = std::find_if(file_stack.begin(), file_stack.end(), [&file_path, &unit](manager::t_file_id frame_file_id) -> bool {
                manager::t_compilation_unit::t_get_file_result frame_get_file_result = unit.get_file(frame_file_id);
                manager::t_compilation_file& frame_file = frame_get_file_result.value();

                return frame_file.path == file_path;

            }) != file_stack.end();

            if (interdependency_found) {
                unit.logger.add_error(file_id, file_path_node.span, std::string("Including \\" + file_path + "\" results in interdependency."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                return;
            }

            manager::t_compilation_unit::t_add_file_result add_file_result = unit.add_file(file_path);
            
            if (add_file_result.has_value())
                file_stack.emplace_back(add_file_result.value());

            // the other error reason (file already exists) should not generate an error.
            // the file was already added, and the fact that nothing new was appended
            // means no compilation was affected.
            else if (add_file_result.error() == manager::t_compilation_unit::t_add_file_error::PATH_INVALID)
                unit.logger.add_error(file_id, file_path_node.span, "The file \\" + file_path + "\" does not exist.");
        }
    }

    void parse_file(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id, std::vector<frontend::manager::t_file_id>& file_stack) {
        using namespace frontend;

        manager::t_compilation_file& file = unit.get_file(file_id).value();

        scan::lexer::lex(unit, file_id);
        scan::parser::parse(unit, file_id);

        handle_file_imports(unit, file_id, file, file_stack);
        
        // could this be written better? probably.
        file.state = manager::t_file_state::ANALYZE_READY;
    }

    void analyze_file(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id) {
        using namespace frontend;

        manager::t_compilation_unit::t_get_file_result get_file_result = unit.get_file(file_id);

        // GET_FILE_RESULT IS CONFIRMED TO EXIST BY THE STACK LOOP THIS FUNCITON IS CALLED IN

        manager::t_compilation_file& file = get_file_result.value();

        sema::semantic_analyzer::analyze(unit, file_id);

        file.state = manager::t_file_state::DONE;
    }
}

std::string frontend::manager::t_log::to_string(const t_compilation_files& files, bool format) const {
    std::stringstream buffer;

    if (format) {
        buffer 
            << util::ansi_format::LIGHT_GRAY
            << '[' 
            << files[file_id.get()].path 
            << " - "
            << span.start.to_string()
            << "]:\n"
            << util::ansi_format::RESET;

        switch (log_type) {
            case t_log_type::MESSAGE:
                buffer << util::ansi_format::CYAN;
                break;
            case t_log_type::WARNING:
                buffer 
                    << util::ansi_format::YELLOW
                    << util::ansi_format::BOLD
                    << "Warning: ";
                break;
            case t_log_type::ERROR:
                buffer 
                    << util::ansi_format::RED
                    << util::ansi_format::BOLD
                    << "Error: ";
                break;
            case t_log_type::INTERNAL_ERROR:
                buffer 
                    << util::ansi_format::RED
                    << util::ansi_format::BOLD
                    << util::ansi_format::UNDERLINE
                    << "Internal Error: ";
                break;
        }
    }
    else {
        buffer << '[' << files[file_id.get()].path << " - " << span.start.to_string() << "]:\n";
        switch (log_type) {
            case t_log_type::MESSAGE:
                break;
            case t_log_type::WARNING:
                buffer << "Warning: ";
                break;
            case t_log_type::ERROR:
                buffer << "Error: ";
                break;
            case t_log_type::INTERNAL_ERROR:
                buffer << "Internal Error: ";
                break;
        }
    }
        
    buffer << message;
    
    if (format)
        buffer << util::ansi_format::RESET;

    return buffer.str();
}

std::string frontend::manager::t_logger::to_string(const t_compilation_files& files, bool format) const {
    std::stringstream buffer;

    buffer << "Logs:\n";
    for (const t_log& log : logs) {
        buffer << log.to_string(files, format) << "\n\n";
    }

    return buffer.str();
}

// base function for parse_file(), analyze_file(), and others
void frontend::manager::t_compilation_unit::process_file(frontend::manager::t_file_id start_file_id) {
    std::vector<manager::t_file_id> file_stack;
    file_stack.emplace_back(start_file_id);

    while (!file_stack.empty()) {
        manager::t_file_id file_id = file_stack.back();

        manager::t_compilation_unit::t_get_file_result get_file_result = get_file(file_id);
        // sanity recheck
        if (!get_file_result.has_value()) {
            file_stack.pop_back();
            continue;
        }

        manager::t_compilation_file& file = get_file_result.value();

        switch (file.state) {
            case manager::t_file_state::PARSE_READY:
                parse_file(*this, file_id, file_stack);
                break;

            case manager::t_file_state::ANALYZE_READY:
                analyze_file(*this, file_id);
                break;

            case manager::t_file_state::DONE:
                file_stack.pop_back();
                std::cout << "Finished frontend for file #" << std::to_string(file_id.get()) << ".\n"; 
                break;
        }
    }
}

// -> t_compilation_unit

frontend::manager::t_compilation_unit::t_add_file_result frontend::manager::t_compilation_unit::add_file(std::string path) {
    bool file_exists = std::find_if(files.begin(), files.end(), [&path](t_compilation_file& file) -> bool {
        return file.path == path;
    }) != files.end();
    
    if (file_exists)
        return std::unexpected(t_compilation_unit::t_add_file_error::FILE_ALREADY_EXISTS);
    
    std::optional<std::string> open_file_result = open_file(path);

    if (!open_file_result.has_value())
        return std::unexpected(t_compilation_unit::t_add_file_error::PATH_INVALID);

    files.emplace_back(path, open_file_result.value());
    return files.size() - 1;
}

frontend::manager::t_compilation_unit::t_get_file_result frontend::manager::t_compilation_unit::get_file(t_file_id file_id) {
    if (static_cast<std::size_t>(file_id) >= files.size())
        return std::nullopt;

    return files.at(static_cast<std::size_t>(file_id));
}

// -> namespace bound

frontend::manager::t_compilation_unit::t_compilation_unit(t_frontend_config _config) 
    : config(std::move(_config)) {
        std::string start_path = config.project_path + '/' + config.start_subpath;
        
        logger.add_log(0, t_log_type::MESSAGE, util::t_span(), std::string("Project path: ") + config.project_path + "\nStart path: " + config.start_subpath);

        t_add_file_result add_file_result = add_file(start_path);

        if (add_file_result.has_value())
            process_file(add_file_result.value());
        else if (add_file_result.error() == t_add_file_error::PATH_INVALID)
            logger.add_error(0, util::t_span(), "Failed to add \"" + start_path + "\" to the compilation unit - path is invalid.");
        
        std::cout << "Compilation finished.\n";
        std::cout << logger.to_string(files);
}