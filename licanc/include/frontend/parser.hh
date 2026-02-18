#pragma once

#include <string>

#include "base.hh"
#include "frontend/ast.hh"

#include "frontend/manager.hh"

namespace frontend::parser {
    void parse(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id);
    void build_test_ast(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id);
}