#include "frontend/sema/semantic_analyzer.hh"

#include "frontend/sema/sym_registrar.hh"
#include "frontend/sema/full_passer.hh"

#include "util/panic.hh"

void frontend::sema::semantic_analyzer::analyze(manager::t_frontend_unit& unit, manager::t_file_id file_id) {
    manager::t_frontend_files::t_get_file_result get_file_result = unit.files.get_file(file_id);
    
    util::panic_assert(
        get_file_result.has_value(), 
        "Symbol registrar attempted to access nonexistent file_id: \"" + std::to_string(file_id.get()) + "\"."
    );

    manager::t_frontend_file& file = get_file_result.value().get();

    file.sym_table.emplace<sym::t_root>();

    sym_registrar::register_symbols({
        .file_id = file_id, 
        .logger = file.logger, 
        .compile_time_data = unit.compile_time_data,
        .files = unit.files,
        .sym_table = file.sym_table, 
        .ast = file.ast
    });

    full_passer::full_pass({
        
    });
}