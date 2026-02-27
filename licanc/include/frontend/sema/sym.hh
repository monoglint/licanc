/*

this file holds all of the struct declarations and definitions for symbols

*/

/*

INSTRUCTIONS FOR MODDING

Add a new t_sym_type value. Create a new independent struct.
Append the new struct type to the variant at the bottom of the file.

semantic note: all of the properties of symbols and nodes don't have an _id even though they're referencing ids. code outside of this file will use an id
suffix to differenciate indexes from actual values. in the context of being in this file, adding _id is just going to be extra useless characters. even though
there is a minor consistency flaw in that decision, i think the upsides outweigh

other semantic note:
the note before each property denotes when that property is set based on the given layer of the analyzer

if there is a format like \* 0, 2 *\
then there are different contexts in which the given property can be set. 
    e.g. when a base function is set vs when a specialization is set.
    - the base function's ast connection can obviously be done in 0,
      but a specialized function being instantiated in a later pass will obviously have it's ast connection set in that later pass and not 0

0 - symbol_registrar
1? - type_resolver
2? - full_passer

^^
THESE ARE NOT FULLY CONCRETELY SET. ONLY USE A NUMBER THAT CORRELATES TO A WORKER THAT IS BEING BUILT OR IS COMPLETE

*/

#pragma once

#include <deque>
#include <unordered_map>
#include <functional>

#include "util/span.hh"
#include "util/vector_hasher.hh"
#include "util/safe_id.hh"
#include "util/hash.hh"

#include "frontend/scan/ast.hh"
#include "frontend/manager_types.hh"

namespace frontend::manager {
    using t_type_name_ids = std::vector<t_type_name_id>;
}

namespace frontend::sema::sym {
    using t_sym_id = util::t_safe_id<struct t_sym_id_tag>;
    using t_sym_ids = std::vector<t_sym_id>;

    using t_declarations = std::unordered_map<manager::t_identifier_id, t_sym_id>;

    // {t_template_argument}
    using t_template_arguments = t_sym_ids;

     // <_, t_x_specialization>
    using t_specializations = std::unordered_map<t_template_arguments, t_sym_id, util::t_vector_hasher<t_sym_id>>;

    // DIRECT MEMBER - NOT A SYMBOL
    struct t_ast_reference {
        scan::ast::t_node_id node_id;
    };



    // SYMBOLS v vvv vv vv
    
    struct t_root { // index 0
        /* 0 */ t_sym_ids file_modules; // {t_module_declaration}
    };

    struct t_template_parameter {
        /* _ */ bool is_constexpr;
        /* _ */ manager::t_constexpr_id constexpr_id;
    };

    struct t_template_argument {
        /* _ */ t_sym_id argument_value; // sema::t_type_name || sema::t_ct_value
    };

    struct t_function {
        /* 0 */ scan::ast::t_node_id syntactic_function;
        /* _ */ manager::t_type_name_ids parameter_types; // {t_type}
        /* _ */ manager::t_type_name_id return_type;
    };
    
    struct t_function_template {
        //  base is not 100% concrete unless len(specializations) is 0 
        //  /
        // v
        /* 0 */ t_sym_id base; // t_function
        /* _ */ t_sym_ids template_parameters; // {t_template_parameter}
        /* _ */ t_specializations specializations; // <_, t_function> 
    };

    struct t_function_declaration {
        /* 0 */ t_sym_id function_template; // t_function_template
    };

    enum class t_access_specifier {
        PUBLIC,
        PRIVATE,
    };

    struct t_property {
        /* _ */ manager::t_type_name_id property_type;
        /* _ */ t_access_specifier access_specifier;
    };

    struct t_method {
        /* _ */ t_sym_id function_template; // t_function_template
        /* _ */ t_access_specifier access_specifier;
    };

    struct t_struct {
        /* 0 */ t_ast_reference syntactic_struct;
        /* _ */ t_declarations properties; // {t_property}
        /* _ */ t_declarations methods; // {t_method}

        // implementation should be fixed here. you can technically access different initializer types by name because a copy constructor
        // will always be called copy, but just be careful. 
        /* _ */ t_declarations initializers; // {t_initializer}
    };

    struct t_struct_template {
        /* 0 */ t_sym_id base; // t_struct
        /* _ */ t_specializations specializations; // <_, t_struct>
        /* _ */ t_sym_ids template_parameters; // {t_template_parameter}
    };

    struct t_struct_declaration {
        /* 0 */ t_sym_id struct_template; // t_struct_template
    };

    struct t_alias_specialization {
        /* _ */ t_ast_reference specialization_node; // ast::t_alias
    };

    struct t_alias_template {
        /* 0 */ t_ast_reference syntactic_alias_template;
        /* _ */ t_specializations specializations;
        /* _ */ t_sym_ids template_parameters; // {t_template_parameter}
    };

    struct t_alias_declaration {
        /* 0 */ t_sym_id alias_template; // t_alias_template
    };

    struct t_module_declaration {
        /* 0 */ t_declarations declarations;
        /* 0 */ t_sym_ids import_markers; // {t_import_marker}
    };

    struct t_global_declaration {
        /* _ */ manager::t_type_name_id global_type;
    };

    struct t_import_marker { 
        // this target module is essentially any module that is a direct child of t_root. 
        t_sym_id target_file_module;
    };

    using t_sym_variation = std::variant<
        t_root,
        t_template_parameter,
        t_template_argument,
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
        t_module_declaration,
        t_global_declaration,
        t_import_marker
    >;

    using t_symbol_table_arena = util::t_arena<t_sym_variation, t_sym_id>;

    struct t_symbol_table : t_symbol_table_arena {
        t_symbol_table() {
            emplace<t_root>();
        }

        [[nodiscard]]
        constexpr inline t_sym_id get_root_id() const {
            return t_sym_id{0}; // always the first thing
        }
    };
}