/*

In previous designs of the lican compiler, a primary core file would link to individual non-core files that held
other data structures like the AST nodes and tokens.

To avoid interdependence, this newer model, though slightly messier, allows for fcore.hh to be included, and
for all of the core components of the compiler (parent ast node class) to be accessed without any interdependence 
whilst all specific components of the compiler (individual ast node classes) are accessed outside of the fcore
folder and namespace.

*/

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "base/base.hh"

#include "ast_fwd.hh"
#include "tok_fwd.hh"

namespace fcore {
    struct s_spot {
        s_spot()
            : start(0), end(0) {}

        s_spot(const u32 start, const u32 end)
            : start(start), end(end) {}

        u32 start;
        u32 end;
    };

    struct s_file_process_state;

    // Underlying state of the entire compiler. Presents the final output of the program.
    struct s_compiler_data {
        base::e_success add_file(const std::string& file_path);

    private:
        // Individual indices denote a unique id for the given file_process_data.
        std::vector<s_file_process_state> file_process_list;
        std::unordered_map<std::string, size_t> file_name_map;
    };

    // frontend/lexer.cc
    base::e_success lex(t_file_process_state& file_process_data);

    // frontend/parser.cc
    // base::e_success parse(t_file_process_data& file_process_data);

    // frontend/sema.cc
    // base::e_success sema(t_file_process_data& file_process_data);
}

struct tok::s_token {
    s_token(const fcore::s_spot& spot, const tok::e_token_type type)
        : spot(spot), type(type) {}
    fcore::s_spot spot;
    tok::e_token_type type;
};

// This struct is the parent of more node structs in "ast_.hh".
struct ast::s_ast_node {
    s_ast_node(const fcore::s_spot& spot, const ast::e_ast_node_type type)
        : spot(spot), type(type) {}

    fcore::s_spot spot;
    ast::e_ast_node_type type;
};

struct fcore::s_file_process_state {
    s_file_process_state(const std::string& source_code)
        : source_code(source_code) {}

    const std::string source_code;
    std::vector<tok::s_token> token_list;
    // base::arena<ast::s_ast_node> ast_arena;
    // base::arena<sym::t_symbol> symbol_arena;  
};