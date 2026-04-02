/*

this file holds all of the struct decls and definitions for symbols

*/

module;

#include <unordered_map>
#include <vector>

export module frontend.sema:syms;

import frontend.ast;

import :ct_expr;
import :resolved_type;
import :syms_fwd;

import util;

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
    using Insts = std::unordered_map<std::vector<util::FirmPtr<TemplateArgument>>, T_INST, util::VectorHasher<util::FirmPtr<TemplateArgument>>>;

    using Decls = std::unordered_map<ast::IdentifierId, util::FirmPtr<Decl>>;

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