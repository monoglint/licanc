module;

#include <string>

export module package;

import frontend.tok;
import frontend.ast;
import frontend.sema;

std::string load_file(std::string file_path) {
    return "empty";
}

export namespace package {
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
        struct PackagePools {
            frontend::ast::IdentifierPool identifier_pool;
            frontend::ast::StringLiteralPool string_literal_pool;
            frontend::sema::ResolvedTypePool resolved_type_pool;
        };

        struct PackageOutput {
            frontend::tok::TokenStream token_stream;
            frontend::ast::AST ast;
            frontend::sema::SymTable sym_table;
        };

        std::string source_code;

        PackageOutput output;
        PackagePools pools;
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