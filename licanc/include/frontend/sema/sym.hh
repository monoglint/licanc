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

#include "util/span.hh"

#include "frontend/scan/ast.hh"

#include "frontend/manager_types.hh"

#include "util/vector_hasher.hh"

namespace frontend::manager {
    using t_type_name_ids = std::vector<t_type_name_id>;
}

namespace frontend::sema::sym {
    using t_sym_id = u64;
    using t_sym_ids = std::vector<t_sym_id>;

    using t_declarations = std::unordered_map<manager::t_identifier_id, t_sym_id>;

     // <{t_template_argument}, t_x_specialization>
    using t_specializations = std::unordered_map<t_sym_ids, t_sym_id, util::vector_hasher<t_sym_id>>;
    
    struct t_root { // index 0
        t_root() {}

        t_declarations declarations;
    };

    struct t_none { // index 1
        t_none() {}
    };
    
    struct t_sym_reference {
        t_sym_reference(manager::t_file_id file_id, t_sym_id sym_id)
            : file_id(file_id), sym_id(sym_id) {}

        manager::t_file_id file_id;
        t_sym_id sym_id;
    };

    struct t_sym_reference_hasher {
        u64 operator()(const t_sym_reference& sym_reference) const noexcept {
            u64 hash_value = std::hash<manager::t_file_id>{}(sym_reference.file_id);
            util::hash_combine(hash_value, std::hash<t_sym_id>{}(sym_reference.sym_id));
            return hash_value;
        }
    };

    struct t_function {
        t_function(scan::ast::t_node_id syntactic_function, manager::t_type_name_ids parameter_types, manager::t_type_name_id return_type)
            : syntactic_function(syntactic_function), parameter_types(parameter_types), return_type(return_type) {}

        scan::ast::t_node_id syntactic_function;
        manager::t_type_name_ids parameter_types; // {t_type}
        manager::t_type_name_id return_type;
    };
    
    struct t_function_template {
        t_function_template(t_sym_id base, t_specializations specializations)
            : base(base), specializations(std::move(specializations)) {}

        t_sym_id base;
        t_specializations specializations; // <_, t_function_specializaiton> 
    };

    struct t_function_declaration {
        t_function_declaration(t_sym_id function_template)
            : function_template(function_template) {}

        t_sym_id function_template; // t_function_template
    };

    enum class t_access_specifier {
        PUBLIC,
        PRIVATE,
    };

    struct t_property {
        t_property(manager::t_type_name_id property_type, t_access_specifier access_specifier)
            : property_type(property_type), access_specifier(access_specifier) {}
        
        manager::t_type_name_id property_type;
        t_access_specifier access_specifier;
    };

    struct t_method {
        t_method(t_sym_id function_template, t_access_specifier access_specifier)
            : function_template(function_template), access_specifier(access_specifier) {}

        t_sym_id function_template; // t_function_template
        t_access_specifier access_specifier;
    };

    struct t_struct {
        t_struct(scan::ast::t_node_id syntactic_struct, t_declarations properties, t_declarations methods, t_declarations initializers)
            : syntactic_struct(syntactic_struct), properties(std::move(properties)), methods(std::move(methods)), initializers(std::move(initializers)) {}

        scan::ast::t_node_id syntactic_struct;
        t_declarations properties; // {t_property}
        t_declarations methods; // {t_method}

        // implementation should be fixed here. you can technically access different initializer types by name because a copy constructor
        // will always be called copy, but just be careful. 
        t_declarations initializers; // {t_initializer}
    };

    struct t_struct_template {
        t_struct_template(t_sym_id base, t_specializations specializations)
            : base(base), specializations(std::move(specializations)) {}

        t_sym_id base; // t_struct
        t_specializations specializations; // <_, t_struct>
    };

    struct t_struct_declaration {
        t_struct_declaration(t_sym_id struct_template)
            : struct_template(struct_template) {}
        
        t_sym_id struct_template; // t_struct_template
    };

    struct t_alias_specialization {
        t_alias_specialization(scan::ast::t_node_id specialization_node)
            : specialization_node(specialization_node) {}

        scan::ast::t_node_id specialization_node; // ast::t_alias
    };

    struct t_alias_template {
        t_alias_template(scan::ast::t_node_id syntactic_alias_template, t_specializations specializations)
            : syntactic_alias_template(syntactic_alias_template), specializations(std::move(specializations)) {}

        scan::ast::t_node_id syntactic_alias_template;
        t_specializations specializations;
    };

    struct t_alias_declaration {
        t_alias_declaration(t_sym_id alias_template)
            : alias_template(alias_template) {}

        t_sym_id alias_template; // t_alias_template
    };

    struct t_module_declaration {
        t_module_declaration(t_declarations declarations)
            : declarations(declarations) {}

        t_declarations declarations; // <t_identifier_id, t_sym_id>
    };

    struct t_global_declaration {
        t_global_declaration(manager::t_type_name_id global_type)
            : global_type(global_type) {}
            
        manager::t_type_name_id global_type;
    };

    using t_sym_variation = std::variant<
        t_root,
        t_none,
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
        t_module_declaration,
        t_global_declaration
    >;

    using t_symbol_table = util::t_arena<t_sym_variation, t_sym_id>;
}