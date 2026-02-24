#pragma once

#include <string>

#include "frontend/sema/sym.hh"

#include "frontend/manager.hh"

/*

the symbol registerer should not fill out struct or function bodies

*/

namespace frontend::sema::symbol_registerer {
    void register_symbols(manager::t_compilation_unit& unit, manager::t_file_id file_id);
}