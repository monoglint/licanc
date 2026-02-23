#pragma once

#include <string>

#include "frontend/sema/sym.hh"

#include "frontend/manager.hh"

namespace frontend::sema::symbol_registerer {
    void register_symbols(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id);
}