export module frontend.parse_pass;

import frontend.ast;

export namespace frontend::parse_pass {
    struct ParserContext {
        
    };

    void run_parser(ParserContext context);
    void build_test_ast(ParserContext context);
}