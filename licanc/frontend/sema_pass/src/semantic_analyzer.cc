module;

#include <string>

module frontend.sema_pass;

import :sym_registrar;
import util;

// void frontend::sema_pass::run_semantic_analyzer(manager::CompilationEngine& engine, manager_types::FileId file_id) {
//     manager::FileManager::GetFileResult get_file_result = engine.file_manager.get_file(file_id);
    
//     util::panic_assert(
//         get_file_result.has_value(), 
//         "Symbol registrar attempted to access nonexistent file_id: \"" + std::to_string(file_id.get()) + "\"."
//     );

//     manager::CompilationFile& file = get_file_result.value().get();

//     sema_pass::run_sym_registrar({
//         .file_id = file_id, 
//         .logger = file.logger, 
//         .session_pools = engine.session_pools,
//         .file_manager = engine.file_manager,
//         .sym_table = file.compiler_output_data.frontend.sym_table, 
//         .ast = file.compiler_output_data.frontend.ast
//     });
// }