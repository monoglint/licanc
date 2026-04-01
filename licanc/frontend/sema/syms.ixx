/*

this file holds all of the struct decls and definitions for symbols

*/

module;

#include <unordered_map>
#include <variant>
#include <vector>

export module frontend.sema:syms;

import :ct_expr;
import :resolved_type;
import :syms_fwd;

import util;

import driver_base;

export namespace frontend::sema {
    struct Sym { };

    enum class DeclKind {
        STRUCT,
        PRIMITIVE,
    };

    struct Decl : Sym {
        Decl(DeclKind decl_kind)
            : decl_kind(decl_kind)
        {}

        DeclKind decl_kind;
    };

    struct TemplateArgument;

    template <class T_INST>
    using Insts = std::unordered_map<std::vector<TemplateArgument*>, T_INST, util::VectorHasher<TemplateArgument*>>;

    using Decls = std::unordered_map<driver_base::IdentifierId, Decl*>;

    // SYMBOLS v vvv vv vv
    
    struct ModuleDecl;
    struct Root : Sym { // index 0
        /* 0 */ ModuleDecl* global_module;
    };

    struct ModuleDecl : Decl {

    };

    struct StructDecl : Decl {

    };

    struct PrimitiveDecl : Decl {

    };

}