#pragma once

#include <string>

#include "frontend/sema/sym.hh"
#include "manager/manager.hh"

/*

the symbol registrar should not fill out struct or function bodies

*/

namespace frontend::sema::sym_registrar {
    struct t_registrar_context {
        manager::t_file_id file_id;
        util::t_logger& logger;
        manager::t_session_pools& session_pools;
        manager::t_file_manager& file_manager;
        sema::sym::t_sym_table& sym_table;
        scan::ast::t_ast& ast;
    };

    void register_symbols(t_registrar_context context);
}