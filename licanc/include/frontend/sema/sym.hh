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

*/

#pragma once

#include <deque>
#include <unordered_map>

#include "util/span.hh"

#include "frontend/scan/ast.hh"

#include "frontend/manager_types.hh"

#include "util/vector_hasher.hh"

namespace frontend::manager {
    using t_type_name_ids = std::vector<t_type_name_id>;
}

namespace frontend::sema::sym {
    using t_sym_id = size_t;
    using t_sym_ids = std::vector<t_sym_id>;

    using t_declarations = std::unordered_map<manager::t_identifier_id, t_sym_id>;

     // <{t_template_argument}, t_x_specialization>
    using t_specializations = std::unordered_map<t_sym_ids, t_sym_id, util::vector_hasher<t_sym_id>>;
    
    struct t_root { // index 0
        t_sym_id global_module; // t_module_declaration
    };

    struct t_none { // index 1
        t_none() {}
    };
    
    struct t_reference {
        manager::t_file_id file_id;
        t_sym_id sym_id;
    };

    struct t_reference_hasher {
        u64 operator()(const t_reference& sym_reference) const noexcept {
            u64 hash_value = std::hash<manager::t_file_id>{}(sym_reference.file_id);
            util::hash_combine(hash_value, std::hash<t_sym_id>{}(sym_reference.sym_id));
            return hash_value;
        }
    };

    struct t_function {
        scan::ast::t_node_id syntactic_function;
        manager::t_type_name_ids parameter_types; // {t_type}
        manager::t_type_name_id return_type;
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
        manager::t_type_name_id property_type;
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
        t_declarations declarations;
        t_sym_ids references; // {t_reference}
    };

    struct t_global_declaration {
        manager::t_type_name_id global_type;
    };

    using t_sym_variation = std::variant<
        t_root,
        t_none,
        t_reference,
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
        t_global_declaration
    >;

    using t_symbol_table = util::t_arena<t_sym_variation, t_sym_id>;
}