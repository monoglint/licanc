/*

This header file is responsible for the internal namespace handling of the compiler.

Some more basic structs from different areas of compilation are declared within this header file.

*/

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "base.hh"

#include "ast_base.hh"
#include "token_base.hh"

namespace fcore {
    // NOTE: Find a shorter name.
    struct t_spot {
        t_spot(const u32 start, const u32 end)
            : start(start), end(end) {}

        u32 start;
        u32 end;
    };

    struct t_token {
        t_token(t_spot&& spot)
            : spot(std::move(spot)) {}
        t_spot spot;
    };

    // This struct is the parent of more node structs in "ast.hh".
    struct t_ast_node {
        t_ast_node(t_spot&& spot)
            : spot(std::move(spot)) {}

        t_spot spot;
    };

    // This struct is the parent of more node structs in "symbol.hh".
    struct t_symbol {
        
    };

    // A temporary state for an individual file in a bigger project.
    struct t_file_process_data {
        std::string source_code;
        std::vector<t_token> token_list;
        base::arena<t_ast_node> ast_arena;
        base::arena<t_symbol> symbol_arena;  
    };

    // Underlying state of the entire compiler. Presents the final output of the program.
    struct t_compiler_data {
        base::t_success add_file(const std::string& file_path);

    private:
        // Individual indices denote a unique id for the given file_process_data.
        std::vector<t_file_process_data> file_list;
        std::unordered_map<std::string, size_t> file_name_map;
    };

    base::t_success lex(t_compiler_data& data);

}
