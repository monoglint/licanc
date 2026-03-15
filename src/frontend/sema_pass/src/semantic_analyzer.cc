module frontend.sema_pass;

import :sym_registrar;
import :full_passer;

import util;


void frontend::sema::semantic_analyzer::run_semantic_analyzer(manager::CompilationEngine& engine, manager::FileId file_id) {
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