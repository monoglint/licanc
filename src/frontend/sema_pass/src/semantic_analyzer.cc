#include "frontend/sema_pass/semantic_analyzer.hh"

#include "frontend/sema_pass/sym_registrar.hh"
#include "frontend/sema_pass/full_passer.hh"

#include "util/panic.hh"

void frontend::sema::semantic_analyzer::analyze(manager::CompilationEngine& engine, manager::FileId file_id) {
    manager::FileManager::GetFileResult get_file_result = engine.file_manager.get_file(file_id);
    
    util::panic_assert(
        get_file_result.has_value(), 
        "Symbol registrar attempted to access nonexistent file_id: \"" + std::to_string(file_id.get()) + "\"."
    );

    manager::CompilationFile& file = get_file_result.value().get();

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