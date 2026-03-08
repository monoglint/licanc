/*

THE SYMBOL REGISTRAR SHOULD ONLY CREATE decls WITH NO TYPE INFO OR ANYTHING EXCEPT CONNECTIONS TO THE AST IF NECESSARY.
ALL SYMBOL REFERENCES NEED TO HOLD FOR A LATER PASS OF THE AST

*/

#include "frontend/sema/sym_registrar.hh"

#include <iostream>

#include "frontend/sema/sema.hh"
#include "util/panic.hh"

// namespace {
//     void walk(RegistrarContext& context, scan::ast::Root* node) {
//         sema::sym::ModuleDecl* global_module_sym = context.sym_table.emplace<sema::sym::ModuleDecl>();
//         context.sym_table.root_ptr->global_module = global_module_sym;

//         eval_decls(context, node->decls, global_module_sym);
//     }
// }

void frontend::sema::sym_registrar::register_symbols(RegistrarContext context) {
    util::panic("The symbol register is not implemented.");
    // walk(context, context.ast.root_ptr);
}