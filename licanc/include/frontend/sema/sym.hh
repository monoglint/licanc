/*

this file holds all of the struct decls and definitions for symbols

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

--
--

note: because properties must be initialized over different passes of the semantic analyzer, default constructors
do not inherently exist (unless enums are being automatically established). it is encouraged that in the future,
a visitor pattern is established to ensure that all symbols have their properties filled in with the context being
based on what pass of the semantic analyzer was just performed

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
        STRUCT,
        MODULE,
        GLOBAL,
        PRIMATIVE,
    };

    struct t_decl : t_sym {
        t_decl(t_decl_type decl_type)
            : decl_type(decl_type) 
        {}

        t_decl_type decl_type;
    };
    
    struct t_template_argument;

    template <class T_INST>
    using t_insts = std::unordered_map<std::vector<t_template_argument*>, T_INST, util::t_vector_hasher<t_template_argument*>>;

    using t_decls = std::unordered_map<manager::t_identifier_id, t_decl*>;

    // SYMBOLS v vvv vv vv
    
    struct t_module_decl;
    struct t_root : t_sym { // index 0
        /* 0 */ t_module_decl* global_module;
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
        /* _ */ t_function* concrete_function;
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
        t_function_decl(t_function_template* function_template)
            : t_decl(t_decl_type::FUNCTION), function_template(function_template)
        {}
            
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

    struct t_struct : t_sym {
        /* 0 */ scan::ast::t_struct* syntactic_struct;
        /* _ */ t_decls properties; // {t_property}
        /* _ */ t_decls methods; // {t_method}

        // implementation should be fixed here. you can technically access different initializer types by name because a copy constructor
        // will always be called copy, but just be careful. 
        /* _ */ t_decls initializers; // {t_initializer}
    };

    struct t_struct_inst {
        t_struct* concrete_struct;
    };

    struct t_struct_template : t_sym {
        /* 0 */ t_struct* base; // t_struct
        /* _ */ t_insts<t_struct_inst*> instantiations;
        /* _ */ std::vector<t_template_parameter*> template_parameters;
    };

    struct t_struct_decl : t_decl {
        t_struct_decl()
            : t_decl(t_decl_type::STRUCT)
        {}

        /* 0 */ t_struct_template* struct_template;
    };

    struct t_primative : t_decl {
        /* 0 */ std::size_t size;
    };

    struct t_primative_decl : t_decl {
        t_primative_decl()
            : t_decl(t_decl_type::PRIMATIVE) 
        {}

        t_primative* primative;
    };

    struct t_import_marker : t_sym {
        // a pointer to the global module of another already semantically analyzed file
        t_module_decl* target_module;
    };

    struct t_module_decl : t_decl {
        t_module_decl(std::optional<t_module_decl*> parent_module, t_decls decls, std::vector<t_import_marker*> import_markers)
            : t_decl(t_decl_type::MODULE), parent_module(std::move(parent_module)), decls(std::move(decls)), import_markers(std::move(import_markers))
        {}

        /* 0 */ std::optional<t_module_decl*> parent_module;
        /* 0 */ t_decls decls;
        /* 0 */ std::vector<t_import_marker*> import_markers;
    };

    struct t_global_decl : t_decl {
        t_global_decl()
            : t_decl(t_decl_type::GLOBAL) {}

        /* _ */ manager::t_type_name_id global_type;
    };

    struct t_sym_table {
        t_sym_table() {}

    private:
        util::t_arena<> arena;

    public:
        t_root* root_ptr; // initialized in semantic_analyzer.cc
        
        template <std::derived_from<t_sym> T, typename... ARGS>
        T* emplace(ARGS... args);

        template <std::derived_from<t_sym> T>
        inline T* push(T sym) {
            return emplace<T, T>(std::move(sym));
        }

        inline void clear() {
            arena.clear();
        }
    };
}