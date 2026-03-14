/*

Takes all of the semantic analyzer workers and condenses them into one function that can be called by the manager.

*/

#pragma once

#include "manager/manager.hh"

namespace frontend::sema::semantic_analyzer {
    void analyze(manager::CompilationEngine& engine, manager::FileId file_id);
}