#pragma once

#include <string>

#include "frontend/scan/token.hh"

#include "frontend/manager.hh"

namespace frontend::scan::lexer {
    void lex(manager::t_compilation_unit& unit, manager::t_file_id file_id);
}