#include "frontend/scan/parser.hh"

#include <filesystem>

namespace frontend::scan::parser {

}


void frontend::scan::parser::parse(t_parser_context context) {
    context.ast.emplace<scan::ast::t_root>();
}

void frontend::scan::parser::build_test_ast(t_parser_context context) {

}