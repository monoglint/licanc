#include "compilation/manager.hh"

#include <fstream>
#include <filesystem>

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

        std::ifstream input_file(file_path);

        if (!input_file || !input_file.is_open())
            return std::unexpected(t_open_file_error::COULDNT_OPEN_FILE);

        return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());    
    }
}

namespace {
    // used by handle_file_imports to mark the current file as an added or existing file's dependency
    // returns the file_id of the new dependency of the given file
    /*
    
    file_id -> file_id of the file that is now a registered dependent of the file who's path is "absolute_file_path"
    
    */
    t_file_id register_file_dependency(manager::t_compilation_engine& engine, manager::t_file_id file_id, const std::string& absolute_file_path) {
        t_file_manager::t_add_file_result add_file_result = engine.file_manager.add_file(absolute_file_path);
        
        if (add_file_result.has_value()) {
            manager::t_file_id new_file_id = add_file_result.value();
            manager::t_compilation_file& new_file = engine.file_manager.get_file(new_file_id).value();

            new_file.dependency_data.dependent_ids.push_back(file_id);

            return new_file_id;
        }

        else if (add_file_result.error() == t_file_manager::t_add_file_error::FILE_ALREADY_EXISTS) {
            manager::t_file_id found_file_id = engine.file_manager.find_file(absolute_file_path).value();
            manager::t_compilation_file& found_file = engine.file_manager.get_file(found_file_id).value();
            
            found_file.dependency_data.dependent_ids.push_back(file_id);

            return found_file_id;
        }
        
        util::panic("An unreachable condition was reached by the file dependency registerer. Whether or not 'absolute_file_path' is valid was checked by the file dependency register's only caller.");
    }

    bool detect_file_interdependency(const manager::t_compilation_engine& engine, manager::t_file_id current_file_id, std::vector<manager::t_file_id>& file_stack, const std::string& absolute_file_path) {
        return std::find_if(file_stack.begin(), file_stack.end(), [&](t_file_id frame_file_id) -> bool {
            if (current_file_id == frame_file_id)
                return false;

            manager::t_file_manager::t_get_const_file_result frame_get_file_result = engine.file_manager.get_file(frame_file_id);
            const t_compilation_file& frame_file = frame_get_file_result.value();

            return frame_file.path == absolute_file_path;

        }) != file_stack.end();
    }
    /*

    lazily loading in all of the files we need should occur after parsing because we have just enough information to know what files we need to use.

    the semantic analyzer will take care of linking references between files together, but actually loading included files can occur
    after parsing. this lessens complexity
    
    */
    void handle_file_imports(manager::t_compilation_engine& engine, manager::t_file_id current_file_id, manager::t_compilation_file& file, std::vector<manager::t_file_id>& file_stack) {
        // check if there is an include item anywhere in the ast.
        for (scan::ast::t_import_decl* import_decl_node : file.compiler_output_data.frontend.ast.import_nodes) {
            if (!import_decl_node->is_path_valid)
                continue;

            manager::t_session_pools::t_string_literal_pool::t_get_result get_string_result = 
                unit.compile_time_data.string_literal_pool.get(import_decl_node->absolute_file_path->string_literal_id);

            const std::string& absolute_file_path = get_string_result.value();

            bool interdependency_found = detect_file_interdependency(unit, current_file_id, file_stack, absolute_file_path);
            if (interdependency_found) {
                file.logger.add_error(import_decl_node->span, std::string("Including \\" + absolute_file_path + "\" results in interdependency (strictly forbidden!!!!)."));

                // although continuing with a caught and non-inserted interdependency will lead to unresolved symbols later on,
                // it is necessary to keep execution going so we can add those errors to the log pool.
                continue;
            }

            manager::t_file_id dependency_file_id = register_file_dependency(unit, current_file_id, absolute_file_path);

            import_decl_node->resolved_file_id = dependency_file_id;
            file_stack.emplace_back(dependency_file_id);
        }
    }

    void parse_file(manager::t_compilation_engine& engine, manager::t_file_id file_id, std::vector<manager::t_file_id>& file_stack) {
        manager::t_compilation_file& file = engine.file_manager.get_file(file_id).value();

        // not implemented yet
        frontend::scan::lexer::lex(frontend::scan::lexer::t_lexer_context{});
        frontend::scan::parser::parse(frontend::scan::parser::t_parser_context{
            .ast = file.compiler_output_data;
        });

        handle_file_imports(unit, file_id, file, file_stack);
    }

    void analyze_file(manager::t_compilation_engine& engine, manager::t_file_id file_id) {
        sema::semantic_analyzer::analyze(engine, file_id);
    }
}
