module;

#include <string>

export module driver;

import frontend.tok;
import frontend.ast;
import frontend.sema;

import driver_base;

std::string load_file(std::string file_path) {
    return "empty";
}

export namespace driver {
    struct ConfigFlags {
        const bool produce_bytecode; // example
    };

    // primary interface struct
    struct Config {
        const std::string source_file_path;
        const ConfigFlags flags;
    };

    //
    //
    //

    struct LoadedPackage {
        std::string source_code;

        frontend::tok::TokenStream token_stream;
        frontend::ast::AST ast;
        frontend::sema::SymTable sym_table;

        driver_base::IdentifierPool identifier_pool;
        driver_base::StringLiteralPool string_literal_pool;
        frontend::sema::ResolvedTypePool resolved_type_pool;
    };

    // primary compiler state struct
    struct CompilerState {
        CompilerState(Config config)

            : config(config),
            loaded_package{
                .source_code = load_file(config.source_file_path)
            }

        {}

        Config config;
        LoadedPackage loaded_package;
    };
};