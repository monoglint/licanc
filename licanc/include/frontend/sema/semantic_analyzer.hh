/*

Takes all of the semantic analyzer workers and condenses them into one function that can be called by the manager.

*/

#pragma once

#include <string>

#include "manager/manager.hh"

namespace frontend::sema::semantic_analyzer {
    void analyze(manager::t_compilation_engine& engine, manager::t_file_id file_id);
}