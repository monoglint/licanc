#pragma once

#include <string>

#include "sym.hh"
#include "ast.hh"

#include "frontend/manager.hh"

namespace frontend::semantic_analyzer {
    void analyze(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id);
}