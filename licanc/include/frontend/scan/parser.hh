#pragma once

#include "frontend/scan/ast.hh"

#include "frontend/manager.hh"

namespace frontend::scan::parser {
    struct t_parser_context {
        ast::t_ast& ast;
    };

    void parse(t_parser_context context);
    void build_test_ast(t_parser_context context);
}