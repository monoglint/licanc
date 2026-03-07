#pragma once

#include "frontend/scan/ast.hh"

namespace frontend::scan::ast_visitor {
    template <class Derived>
    class DeclVisitor {
        inline void call() {

        }
        
    private:
        inline Derived& get_derived() {
            return static_cast<Derived*>(this);
        }

        inline void visit_import(ast::Root* root) { }
        inline void visit_global(ast::Root* root) { }
        inline void visit_function(ast::Root* root) { }
        inline void visit_struct(ast::Root* root) { }
        inline void visit_module(ast::Root* root) { }
    };

    class VisitImportDecl : DeclVisitor<VisitImportDecl> {
        virtual void VisitRoot();
    };
}