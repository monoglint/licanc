module;

#include <string>
#include <iostream>

module driver;

import frontend.lex_pass;
import frontend.parse_pass;
import frontend.sema_pass;

namespace {
    std::string load_file(std::string file_path) {
        return "empty";
    }
}

void driver::compile(Config config) {
    CompilerState compiler_state {
        .loaded_package = {
            .source_code = load_file(config.source_file_path),
            // token_stream, ast, symbol_table auto initialized
        },
        .config = config,   
    };

    std::cout << "Running passes\n";

    frontend::lex_pass::run_lexer({});
    frontend::parse_pass::run_parser({});
    frontend::sema_pass::run_semantic_analyzer({});
}