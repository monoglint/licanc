/*

this file holds all of the struct declarations and definitions for symbols

*/

/*

INSTRUCTIONS FOR MODDING

Add a new t_sym_type value. Create a new independent struct.
Append the new struct type to the variant at the bottom of the file.

*/

#pragma once

#include <deque>
#include <unordered_map>

#include "base.hh"

#include "frontend/scan/ast.hh"

#include "frontend/manager_types.hh"

#include "util/vector_hasher.hh"

namespace frontend::sema::sym {
    using t_sym_id = u64;
    using t_sym_ids = std::vector<t_sym_id>;

    using t_declarations = std::unordered_map<manager::t_identifier_id, t_sym_id>;

     // <{t_template_argument}, t_x_specialization>
    using t_specializations = std::unordered_map<t_sym_ids, t_sym_id, util::vector_hasher<t_sym_id>>;
    
    struct t_sym_reference {
        manager::t_file_id file_id;
        t_sym_id sym_id;
    };

    struct t_function {
        scan::ast::t_node_id syntactic_function;
        t_sym_ids parameter_types; // {t_type}
        t_sym_id return_type; // t_type
    };
    
    struct t_function_template {
        t_sym_id base;
        t_specializations specializations; // <_, t_function_specializaiton> 
    };

    struct t_function_declaration {
        t_sym_id function_template; // t_function_template
    };

    enum class t_access_specifier {
        PUBLIC,
        PRIVATE,
    };

    struct t_property {
        t_sym_id property_type; // t_type_wrapper
        t_access_specifier access_specifier;
    };

    struct t_method {
        t_sym_id function_template; // t_function_template
        t_access_specifier access_specifier;
    };

    struct t_struct {
        scan::ast::t_node_id syntactic_struct;
        t_declarations properties; // {t_property}
        t_declarations methods; // {t_method}

        // implementation should be fixed here. you can technically access different initializer types by name because a copy constructor
        // will always be called copy, but just be careful. 
        t_declarations initializers; // {t_initializer}
    };

    struct t_struct_template {
        t_sym_id base; // t_struct
        t_specializations specializations; // <_, t_struct>
    };

    struct t_struct_declaration {
        t_sym_id struct_template; // t_struct_template
    };

    struct t_alias_specialization {
        scan::ast::t_node_id specialization_node; // ast::t_alias
    };

    struct t_alias_template {
        scan::ast::t_node_id syntactic_alias_template;
        t_specializations specializations;
    };

    struct t_alias_declaration {
        t_sym_id alias_template; // t_alias_template
    };

    struct t_module_declaration {
        t_declarations declarations; // <t_identifier_id, t_sym_id>
    };

    using t_sym_variation = std::variant<
        t_sym_reference,
        t_function,
        t_function_template,
        t_function_declaration,
        t_property,
        t_method,
        t_struct,
        t_struct_template,
        t_struct_declaration,
        t_alias_specialization,
        t_alias_template,
        t_alias_declaration,
        t_module_declaration
    >;

    using t_symbol_table = util::t_variant_deque<t_sym_variation, t_sym_id>;
}