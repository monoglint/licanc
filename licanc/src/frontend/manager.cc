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
        using namespace frontend;

        // check if there is an include item anywhere in the ast.
        for (std::size_t index = 0; index < file.ast.get_size(); index++) {
            scan::ast::t_node_id import_node_id(index);

            if (!file.ast.is<scan::ast::t_import_item>(import_node_id))
                continue;

            scan::ast::t_import_item& import_item_node = file.ast.get<scan::ast::t_import_item>(import_node_id).value().get(); // explicit bypass <-------__
            scan::ast::t_string_literal& file_path_node = file.ast.get<scan::ast::t_string_literal>(import_item_node.file_path).value().get(); //           \        -no multiline
            
            auto get_string_result = unit.compile_time_data.string_literal_pool.get(file_path_node.string_literal_id);
            std::string& file_path = get_string_result.value(); // explicit bypass

            // we're calling the file frame_file since we're iterating through indexes of a stack
            bool interdependency_found = std::find_if(file_stack.begin(), file_stack.end(), [&file_path, &unit](manager::t_file_id frame_file_id) -> bool {
                manager::t_compilation_files::t_get_file_result frame_get_file_result = unit.files.get_file(frame_file_id);
                manager::t_compilation_file& frame_file = frame_get_file_result.value();

                return frame_file.path == file_path;

            }) != file_stack.end();

            if (interdependency_found) {
                unit.logger.add_error(file_id, file_path_node.span, std::string("Including \\" + file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                return;
            }

            manager::t_compilation_files::t_add_file_result add_file_result = unit.files.add_file(file_path);
            
            if (add_file_result.has_value()) {
                file_stack.emplace_back(add_file_result.value());
                import_item_node.resolved_file_id = add_file_result.value();
            }
            else if (add_file_result.error() == manager::t_compilation_files::t_add_file_error::FILE_ALREADY_EXISTS)
                import_item_node.resolved_file_id = unit.files.find_file(file_path).value(); // explicit bypass

            else if (add_file_result.error() == manager::t_compilation_files::t_add_file_error::PATH_INVALID)
                unit.logger.add_error(file_id, file_path_node.span, "The file \\" + file_path + "\" does not exist.");
        }
    }

    void parse_file(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id, std::vector<frontend::manager::t_file_id>& file_stack) {
        frontend::manager::t_compilation_file& file = unit.files.get_file(file_id).value();

        frontend::scan::lexer::lex(frontend::scan::lexer::t_lexer_context{});

        frontend::scan::parser::parse(frontend::scan::parser::t_parser_context{});

        handle_file_imports(unit, file_id, file, file_stack);
    }

    void analyze_file(frontend::manager::t_compilation_unit& unit, frontend::manager::t_file_id file_id) {
        frontend::manager::t_compilation_files::t_get_file_result get_file_result = unit.files.get_file(file_id);

        // GET_FILE_RESULT IS CONFIRMED TO EXIST BY THE STACK LOOP THIS FUNCITON IS CALLED IN

        frontend::sema::semantic_analyzer::analyze(unit, file_id);
    }
}

std::string frontend::manager::t_log::to_string(const t_compilation_files& files, bool format) const {
    std::stringstream buffer;

    t_compilation_files::t_get_const_file_result get_file_result = files.get_file(file_id);

    const std::string source_file_path = get_file_result.has_value() ? get_file_result.value().get().path : "[???]";

    if (format) {
        buffer 
            << util::ansi_format::LIGHT_GRAY
            << '[' 
            << source_file_path
            << " - "
            << span.start.to_string()
            << "]:\n"
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
            case t_log_type::INTERNAL_ERROR:
                buffer << util::ansi_format::RED << util::ansi_format::BOLD << util::ansi_format::UNDERLINE << "Internal Error: ";
                break;
        }
    }
    else {
        buffer << '[' << source_file_path << " - " << span.start.to_string() << "]:\n";
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

        manager::t_compilation_files::t_get_file_result get_file_result = files.get_file(file_id);
        // sanity recheck
        if (!get_file_result.has_value()) {
            file_stack.pop_back();
            continue;
        }

        manager::t_compilation_file& file = get_file_result.value();

        switch (file.state) {
            case manager::t_file_state::SCAN_READY:
                parse_file(*this, file_id, file_stack);
                file.state = frontend::manager::t_file_state::SEMA_READY;
                break;

            case manager::t_file_state::SEMA_READY:
                analyze_file(*this, file_id);
                file.state = frontend::manager::t_file_state::DONE;
                break;

            case manager::t_file_state::DONE:
                file_stack.pop_back();
                std::cout << "Finished frontend for file #" << std::to_string(file_id.get()) << ".\n"; 
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

// -> namespace bound

frontend::manager::t_compilation_unit::t_compilation_unit(t_frontend_config _config) 
    : config(std::move(_config)) {
    std::string start_path = config.project_path + '/' + config.start_subpath;
    
    logger.add_log(t_file_id::INVALID_ID, t_log_type::MESSAGE, util::t_span(), std::string("Project path: ") + config.project_path + "\nStart path: " + config.start_subpath);

    t_compilation_files::t_add_file_result add_file_result = files.add_file(start_path);

    if (add_file_result.has_value())
        process_file(add_file_result.value());
    else if (add_file_result.error() == t_compilation_files::t_add_file_error::PATH_INVALID)
        logger.add_error(t_file_id{0}, util::t_span(), "Failed to add \"" + start_path + "\" to the compilation unit - path is invalid.");
    
    std::cout << "Compilation finished.\n";
    std::cout << logger.to_string(files);
}