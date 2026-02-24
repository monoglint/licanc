#pragma once

#include <string>

#include "frontend/scan/ast.hh"

#include "frontend/manager.hh"

namespace frontend::scan::parser {
    void parse(manager::t_compilation_unit& unit, manager::t_file_id file_id);
    void build_test_ast(manager::t_compilation_unit& unit, manager::t_file_id file_id);
}