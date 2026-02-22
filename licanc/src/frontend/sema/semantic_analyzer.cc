#include "frontend/sema/semantic_analyzer.hh"

#include "frontend/sema/symbol_registerer.hh"
#include "frontend/sema/full_passer.hh"

void frontend::sema::semantic_analyzer::analyze(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id) {
    symbol_registerer::register_symbols(compilation_unit, file_id);
    full_passer::full_pass(compilation_unit, file_id);
}