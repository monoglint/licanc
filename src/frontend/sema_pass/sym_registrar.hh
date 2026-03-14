#pragma once

#include <string>

#include "frontend/sema/sym.hh"
#include "manager/manager.hh"

/*

the symbol registrar should not fill out struct or function bodies

*/

namespace frontend::sema::sym_registrar {
    struct RegistrarContext {
        manager::FileId file_id;
        util::Logger& logger;
        manager::SessionPools& session_pools;
        manager::FileManager& file_manager;
        sema::sym::SymTable& sym_table;
        scan::ast::AST& ast;
    };

    void register_symbols(RegistrarContext context);
}