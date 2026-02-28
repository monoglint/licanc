#include "frontend/sema/semantic_analyzer.hh"

#include "frontend/sema/sym_registrar.hh"
#include "frontend/sema/full_passer.hh"

#include "util/panic.hh"

void frontend::sema::semantic_analyzer::analyze(manager::t_compilation_unit& unit, manager::t_file_id file_id) {
    manager::t_compilation_files::t_get_file_result get_file_result = unit.files.get_file(file_id);
    
    if (!get_file_result.has_value()) {
        util::panic("Symbol registrar attempted to access nonexistent file_id: \"" + std::to_string(file_id.get()) + "\".");
        return;
    }

    manager::t_compilation_file& file = get_file_result.value().get();

    sym_registrar::register_symbols({
        .file_id = file_id, 
        .logger = unit.logger, 
        .compile_time_data = unit.compile_time_data, 
        .sym_table = unit.sym_table, 
        .ast = file.ast
    });

    full_passer::full_pass({
        
    });
}