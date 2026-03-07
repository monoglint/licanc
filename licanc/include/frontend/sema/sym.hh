/*

this file holds all of the struct decls and definitions for symbols

*/

/*

INSTRUCTIONS FOR MODDING

Add a new SymType value. Create a new independent struct.
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
#include "manager/manager_types.hh"

namespace frontend::sema::sym {
    struct Sym { };

    enum class DeclType {
        FUNCTION,
        STRUCT,
        MODULE,
        GLOBAL,
        PRIMATIVE,
    };

    struct Decl : Sym {
        Decl(DeclType decl_type)
            : decl_type(decl_type)
        {}

        DeclType decl_type;
    };
    
    struct TemplateArgument;

    template <class T_INST>
    using Insts = std::unordered_map<std::vector<TemplateArgument*>, T_INST, util::VectorHasher<TemplateArgument*>>;

    using Decls = std::unordered_map<manager::IdentifierId, Decl*>;

    // SYMBOLS v vvv vv vv
    
    struct ModuleDecl;
    struct Root : Sym { // index 0
        /* 0 */ ModuleDecl* global_module;
    };

    struct TypeNameTemplateParameter : Sym {

    };

    struct ValueTemplateParameter : Sym {
        /* _ */ manager::ConstexprId constexpr_id;
    };

    using TemplateParameterVariant = std::variant<TypeNameTemplateParameter*, ValueTemplateParameter*>;
    struct TemplateParameter : Sym {
        /* _ */ TemplateParameterVariant value;
    };

    //                                                                   not definite yet, just here for implementation
    using TemplateArgumentValueVariant = std::variant<manager::ResolvedTypeId, manager::ConstexprId>;
    struct TemplateArgument : Sym {
        /* _ */ TemplateArgumentValueVariant argument_value;
    };

    struct Function : Sym {
        /* 0 */ scan::ast::Function* syntactic_function;
        /* _ */ std::vector<manager::ResolvedTypeId> parameter_types; // {Type}
        /* _ */ manager::ResolvedTypeId return_type;
    };

    struct FunctionInst {
        /* _ */ Function* concrete_function;
    };
    
    struct FunctionTemplate : Sym {
        //  base is not 100% concrete unless len(specializations) is 0 
        //  /
        // v
        /* 0 */ Function* base; // Function
        /* _ */ std::vector<TemplateParameter*> template_parameters; // {TemplateParameter}
        /* _ */ Insts<FunctionInst*> instantiations; // <_, Function> 
    };

    struct FunctionOverload : Sym {
        /* _ */ std::vector<manager::ResolvedTypeId> parameter_types;
        /* _ */ FunctionTemplate* function_template; // FunctionTemplate
    };

    struct FunctionDecl : Decl {
        FunctionDecl(std::vector<FunctionOverload*> overloads)
            : Decl(DeclType::FUNCTION), overloads(std::move(overloads))
        {}
            
        // default overload stored in index 0
        /* index 0: 0 */ std::vector<FunctionOverload*> overloads;
    };

    enum class AccessSpecifier {
        PUBLIC,
        PRIVATE,
    };

    struct Property : Sym {
        /* _ */ manager::ResolvedTypeId property_type;
        /* _ */ AccessSpecifier access_specifier;
    };

    struct Method : Sym {
        /* _ */ FunctionTemplate* function_template; // FunctionTemplate
        /* _ */ AccessSpecifier access_specifier;
    };

    struct Struct : Sym {
        /* 0 */ scan::ast::Struct* syntactic_struct;
        /* _ */ Decls properties; // {Property}
        /* _ */ Decls methods; // {Method}

        // implementation should be fixed here. you can technically access different initializer types by name because a copy constructor
        // will always be called copy, but just be careful. 
        /* _ */ Decls initializers; // {Initializer}
    };

    struct StructInst {
        Struct* concrete_struct;
    };

    struct StructTemplate : Sym {
        /* 0 */ Struct* base; // Struct
        /* _ */ Insts<StructInst*> instantiations;
        /* _ */ std::vector<TemplateParameter*> template_parameters;
    };

    struct StructDecl : Decl {
        StructDecl()
            : Decl(DeclType::STRUCT)
        {}

        /* 0 */ StructTemplate* struct_template;
    };

    struct Primative : Decl {
        /* 0 */ std::size_t size;
    };

    struct PrimativeDecl : Decl {
        PrimativeDecl()
            : Decl(DeclType::PRIMATIVE)
        {}

        Primative* primative;
    };

    struct ImportMarker : Sym {
        // a pointer to the global module of another already semantically analyzed file
        ModuleDecl* target_module;
    };

    struct ModuleDecl : Decl {
        ModuleDecl(std::optional<ModuleDecl*> parent_module, Decls decls, std::vector<ImportMarker*> import_markers)
            : Decl(DeclType::MODULE), parent_module(std::move(parent_module)), decls(std::move(decls)), import_markers(std::move(import_markers))
        {}

        /* 0 */ std::optional<ModuleDecl*> parent_module;
        /* 0 */ Decls decls;
        /* 0 */ std::vector<ImportMarker*> import_markers;
    };

    struct GlobalDecl : Decl {
        GlobalDecl()
            : Decl(DeclType::GLOBAL) {}

        /* _ */ manager::ResolvedTypeId global_type;
    };

    class SymTable {
    public:
        SymTable() { init(); }

        Root* root_ptr; // initialized in semantic_analyzer.cc
        
        template <std::derived_from<Sym> T, typename... ARGS>
        T* emplace(ARGS... args);

        template <std::derived_from<Sym> T>
        inline T* push(T sym) {
            return emplace<T, T>(std::move(sym));
        }

        inline void clear() {
            arena.clear();
            init();
        }
        
    private:
        util::Arena<> arena;

        inline void init() {
            std::ignore = arena.emplace<Root>();
        }
    };
}