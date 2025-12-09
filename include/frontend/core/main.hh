/*

This header file is responsible for the internal namespace handling of the compiler.

Some more basic structs from different areas of compilation are declared within this header file.

*/

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "base.hh"

#include "ast_top.hh"
#include "token_top.hh"

namespace fcore {
    struct t_spot {
        t_spot(const u32 start, const u32 end)
            : start(start), end(end) {}

        u32 start;
        u32 end;
    };

    struct t_file_process_data;

    // Underlying state of the entire compiler. Presents the final output of the program.
    struct t_compiler_data {
        base::t_success add_file(const std::string& file_path);

    private:
        // Individual indices denote a unique id for the given file_process_data.
        std::vector<t_file_process_data> file_list;
        std::unordered_map<std::string, size_t> file_name_map;
    };

    // base::t_success lex(t_file_process_data& data);
    // base::t_success parse(t_file_process_data& data);
    // base::t_success sema(t_file_process_data& data);
}

struct tok::t_token {
    t_token(fcore::t_spot&& spot, const tok::t_token_type type)
        : spot(std::move(spot)), type(type) {}
    fcore::t_spot spot;
    t_token_type type;
};


// This struct is the parent of more node structs in "ast.hh".
struct ast::t_ast_node {
    t_ast_node(fcore::t_spot&& spot, const ast::t_ast_node_type type)
        : spot(std::move(spot)), type(type) {}

    fcore::t_spot spot;
    t_ast_node_type type;
};

struct fcore::t_file_process_data {
    t_file_process_data(std::string&& source_code)
        : source_code(std::move(source_code)) {}

    std::string source_code;
    std::vector<tok::t_token> token_list;
    base::arena<ast::t_ast_node> ast_arena;
    // base::arena<sym::t_symbol> symbol_arena;  
};