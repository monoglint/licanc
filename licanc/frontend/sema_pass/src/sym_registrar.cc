/*

THE SYMBOL REGISTRAR SHOULD ONLY CREATE decls WITH NO TYPE INFO OR ANYTHING EXCEPT CONNECTIONS TO THE AST IF NECESSARY.
ALL SYMBOL REFERENCES NEED TO HOLD FOR A LATER PASS OF THE AST

*/

module frontend.sema_pass;

import util;

// namespace {
//     void walk(RegistrarContext& context, ast::Root* node) {
//         sema::ModuleDecl* global_module_sym = context.sym_table.emplace<sema::ModuleDecl>();
//         context.sym_table.root_ptr->global_module = global_module_sym;

//         eval_decls(context, node->decls, global_module_sym);
//     }
// }

void frontend::sema_pass::run_sym_registrar(SymRegistrarContext context) {
    util::panic("The symbol register is not implemented.");
    // walk(context, context.ast.root_ptr);
}