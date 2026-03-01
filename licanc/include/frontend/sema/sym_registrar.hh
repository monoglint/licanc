#pragma once

#include <string>

#include "frontend/sema/sym.hh"

#include "frontend/manager.hh"

/*

the symbol registrar should not fill out struct or function bodies

*/

namespace frontend::sema::sym_registrar {
    struct t_registrar_context {
        manager::t_file_id file_id;
        manager::t_logger& logger;
        manager::t_compile_time_data& compile_time_data;
        manager::t_compilation_files& files;
        sema::sym::t_sym_table& sym_table;
        scan::ast::t_ast& ast;
    };

    void register_symbols(t_registrar_context context);
}