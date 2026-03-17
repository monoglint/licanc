module;

#include <string>

module driver;

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
}