#include "frontend/sema/semantic_analyzer.hh"

#include "frontend/sema/sym_registrar.hh"
#include "frontend/sema/full_passer.hh"

#include "util/panic.hh"

void frontend::sema::semantic_analyzer::analyze(manager::t_compilation_engine& engine, manager::t_file_id file_id) {
    manager::t_file_manager::t_get_file_result get_file_result = engine.file_manager.get_file(file_id);
    
    util::panic_assert(
        get_file_result.has_value(), 
        "Symbol registrar attempted to access nonexistent file_id: \"" + std::to_string(file_id.get()) + "\"."
    );

    manager::t_compilation_file& file = get_file_result.value().get();

    sym_registrar::register_symbols({
        .file_id = file_id, 
        .logger = file.logger, 
        .session_pools = engine.session_pools,
        .file_manager = engine.file_manager,
        .sym_table = file.compiler_output_data.frontend.sym_table, 
        .ast = file.compiler_output_data.frontend.ast
    });

    full_passer::full_pass({
        
    });
}