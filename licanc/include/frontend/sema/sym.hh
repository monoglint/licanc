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

0 - sym_registrar
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
    struct t_sym { };

    enum class t_decl_type {
        FUNCTION,
        RECORD,
        MODULE,
        GLOBAL,
    };

    struct t_decl : t_sym {
        t_decl(t_decl_type decl_type)
            : decl_type(decl_type) {}

        t_decl_type decl_type;
    };
    
    struct t_template_argument;

    template <class T_INST>
    using t_insts = std::unordered_map<std::vector<t_template_argument*>, T_INST, util::t_vector_hasher<t_template_argument*>>;

    using t_decls = std::unordered_map<manager::t_identifier_id, t_decl*>;

    // SYMBOLS v vvv vv vv
    
    struct t_module_decl;
    struct t_root : t_sym { // index 0
        /* 0 */ std::vector<t_module_decl*> file_modules; // {t_module_decl}
    };

    struct t_type_name_template_parameter : t_sym {

    };

    struct t_value_template_parameter : t_sym {
        /* _ */ manager::t_constexpr_id constexpr_id;
    };

    using t_template_parameter_variant = std::variant<t_type_name_template_parameter*, t_value_template_parameter*>;
    struct t_template_parameter : t_sym {
        /* _ */ t_template_parameter_variant value;
    };

    //                                                                   not definite yet, just here for implementation
    using t_template_argument_value_variant = std::variant<manager::t_type_name_id, manager::t_constexpr_id>;
    struct t_template_argument : t_sym {
        /* _ */ t_template_argument_value_variant argument_value;
    };

    struct t_function : t_sym {
        /* 0 */ scan::ast::t_function* syntactic_function;
        /* _ */ manager::t_type_name_ids parameter_types; // {t_type}
        /* _ */ manager::t_type_name_id return_type;
    };

    struct t_function_inst {
        /* _ */ t_function* function;
    };
    
    struct t_function_template : t_sym {
        //  base is not 100% concrete unless len(specializations) is 0 
        //  /
        // v
        /* 0 */ t_function* base; // t_function
        /* _ */ std::vector<t_template_parameter*> template_parameters; // {t_template_parameter}
        /* _ */ t_insts<t_function_inst*> instantiations; // <_, t_function> 
    };

    struct t_function_decl : t_decl {
        t_function_decl()
            : t_decl(t_decl_type::FUNCTION) {}
            
        /* 0 */ t_function_template* function_template; // t_function_template
    };

    enum class t_access_specifier {
        PUBLIC,
        PRIVATE,
    };

    struct t_property : t_sym {
        /* _ */ manager::t_type_name_id property_type;
        /* _ */ t_access_specifier access_specifier;
    };

    struct t_method : t_sym {
        /* _ */ t_function_template* function_template; // t_function_template
        /* _ */ t_access_specifier access_specifier;
    };

    struct t_record : t_sym {
        /* 0 */ scan::ast::t_record* syntactic_record;
        /* _ */ t_decls properties; // {t_property}
        /* _ */ t_decls methods; // {t_method}

        // implementation should be fixed here. you can technically access different initializer types by name because a copy constructor
        // will always be called copy, but just be careful. 
        /* _ */ t_decls initializers; // {t_initializer}
    };

    struct t_record_inst {
        t_record* record;
    };

    struct t_record_template : t_sym {
        /* 0 */ t_record* base; // t_record
        /* _ */ t_insts<t_record_inst*> instantiations;
        /* _ */ std::vector<t_template_parameter*> template_parameters;
    };

    struct t_record_decl : t_decl {
        /* 0 */ t_record_template* record_template;
    };

    struct t_primative : t_decl {
        /* 0 */ std::size_t size;
    };

    struct t_primative_decl : t_decl {
        t_primative* primative;
    };

    struct t_import_marker : t_sym { 
        // this target module is essentially any module that is a direct child of t_root. 
        t_module_decl* get_file_module;
    };

    struct t_module_decl : t_decl {
        /* 0 */ t_module_decl* parent_module;
        /* 0 */ t_decls declarations;
        /* 0 */ std::vector<t_import_marker*> import_markers;
    };

    struct t_global_decl : t_decl {
        /* _ */ manager::t_type_name_id global_type;
    };

    struct t_sym_table {
        t_sym_table()
            : root_ptr(arena.emplace<t_root>()) {}

    private:
            util::t_arena<> arena;

    public:
        t_root* root_ptr;
    
        template <std::derived_from<t_sym> T>
        inline T* push(T sym) {
            T* ptr = arena.push<T>(std::move(sym));

            return ptr; 
        } 
    };
}