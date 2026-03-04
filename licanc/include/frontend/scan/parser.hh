#pragma once

#include "frontend/scan/ast.hh"

#include "frontend/manager.hh"

namespace frontend::scan::parser {
    struct ParserContext {
        ast::AST& ast;
    };

    void parse(ParserContext context);
    void build_test_ast(ParserContext context);
}