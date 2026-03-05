#pragma once

#include "frontend/scan/ast.hh"

namespace frontend::scan::ast_visitor {
    template <class Derived>
    class ASTVisitor {

    };

    class ASTConsumer {
        virtual void VisitRoot();
    };



    class ASTWalker : public ASTVisitor<ASTWalker> {

    };
}