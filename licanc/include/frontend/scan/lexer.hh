#pragma once

#include <string>

#include "base.hh"
#include "frontend/scan/token.hh"

#include "frontend/manager.hh"

namespace frontend::scan::lexer {
    void lex(manager::t_compilation_unit& compilation_unit, manager::t_file_id file_id);
}