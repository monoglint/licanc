#include "frontend/sema/semantic_analyzer.hh"

#include "frontend/sema/symbol_registrar.hh"
#include "frontend/sema/full_passer.hh"

void frontend::sema::semantic_analyzer::analyze(manager::t_compilation_unit& unit, manager::t_file_id file_id) {
    manager::t_compilation_files::t_get_file_result get_file_result = unit.files.get_file(file_id);
    
    if (!get_file_result.has_value()) {
        unit.logger.add_internal_error(file_id, util::t_span(), "Symbol registrar attempted to access nonexistent file_id: \"" + std::to_string(file_id.get()) + "\".");
        return;
    }

    manager::t_compilation_file& file = get_file_result.value().get();

    symbol_registrar::register_symbols({
        .file_id = file_id, 
        .logger = unit.logger, 
        .compile_time_data = unit.compile_time_data, 
        .symbol_table = unit.symbol_table, 
        .ast = file.ast
    });

    full_passer::full_pass({
        
    });
}