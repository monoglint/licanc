/*

this file holds all of the struct declarations and definitions for symbols

*/

/*

INSTRUCTIONS FOR MODDING

Add a new t_sym_type value. Create a new independent struct.
Append the new struct type to the variant at the bottom of the file.

ENSURE THAT THE ORDER OF THE TYPE IN THE VARIANT LIST IS CONGRUENT TO THE ORDER OF THE ENUMS.
THAT IS HOW SYMBOL TYPE IS IDENTIFIED!

*/

#pragma once

#include <deque>
#include <unordered_map>

#include "base.hh"
#include "frontend/ast.hh"

namespace frontend::sym {
    // spec = specialized (instantiated)
    // decl = declaration (templated / uninstantiated)
    // sym = uncategorized symbol

    enum class t_sym_type {
        SYM_TYPE_WRAPPER,

        SYM_REFERENCE,
        SYM_TYPE_PARAMETER,

        SPEC_FUNCTION,
 
        DECL_FUNCTION,
        DECL_MODULE,
    };

    enum class t_type_parameter_specifier {
        TYPE_NAME, // template <typename T>
        VALUE, // template <int A>
    };

    using t_sym_id = u64;
    using t_sym_ids = std::vector<t_sym_id>;

    struct t_sym_type_wrapper {
        t_sym_id wrapee_sym_id;
        ast::t_type_qualifier qualifier;
    };

    struct t_sym_reference {
        u64 file_id; // do not reference another header just for a type name that is an alias of u64 lol. no matter what, we'll store highest precision just in case
        t_sym_id sym_id;
    };

    struct t_sym_type_parameter {
        t_sym_id value; // (specifier == TYPENAME) ? (string??) : (t_res_type) 
        t_type_parameter_specifier specifier;
    };

    struct t_spec_function {
        t_sym_id return_type_id;
    };

    struct t_decl_function {
        t_sym_ids overload_ids; // 
    };

    struct t_decl_module {
        std::unordered_map<u64, t_sym_id> declaration_ids; // t_identifier_id, t_sym_id
    };

    struct t_symbol_table {

    };
}