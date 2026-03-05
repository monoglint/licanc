#pragma once

#include "frontend/scan/ast.hh"
#include "manager/manager.hh"

namespace frontend::scan::parser {
    struct ParserContext {
        ast::AST& ast;
        manager::EngineContext engine_context;
    };

    void parse(ParserContext context);
    void build_test_ast(ParserContext context);
}