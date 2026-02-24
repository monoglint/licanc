#include "frontend/sema/semantic_analyzer.hh"

#include "frontend/sema/symbol_registerer.hh"
#include "frontend/sema/full_passer.hh"

void frontend::sema::semantic_analyzer::analyze(manager::t_compilation_unit& unit, manager::t_file_id file_id) {
    manager::t_compilation_unit::t_get_file_result get_file_result = unit.get_file(file_id);
    
    if (!get_file_result.has_value()) {
        unit.logger.add_internal_error(file_id, util::t_span(), "Symbol registerer attempted to access nonexistent file_id: \"" + std::to_string(file_id) + "\".");
        return;
    }

    manager::t_compilation_file& file = get_file_result.value();
    file.symbol_table.emplace<sym::t_root>(); // index 0
    file.symbol_table.emplace<sym::t_none>(); // index 1

    symbol_registerer::register_symbols(unit, file_id);
    full_passer::full_pass(unit, file_id);
}