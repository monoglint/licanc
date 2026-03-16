export module frontend.sema_pass:sym_registrar;

import frontend.ast;
import frontend.sema;

/*

the symbol registrar should not fill out struct or function bodies

*/

export namespace frontend::sema_pass {
    struct SymRegistrarContext {
        sema::SymTable& sym_table;
        ast::AST& ast;
    };

    void run_sym_registrar(SymRegistrarContext context);
}