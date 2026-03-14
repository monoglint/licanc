#pragma once

#include "frontend/ast/ast.hh"

namespace frontend::ast::ast_visitor {
    template <class Derived>
    class DeclVisitor {
        void call() {

        }
        
    private:
        Derived& get_derived() {
            return static_cast<Derived*>(this);
        }

        void visit_import(ast::Root* root) { }
        void visit_global(ast::Root* root) { }
        void visit_function(ast::Root* root) { }
        void visit_struct(ast::Root* root) { }
        void visit_module(ast::Root* root) { }
    };

    class VisitImportDecl : DeclVisitor<VisitImportDecl> {
        virtual void VisitRoot();
    };
}