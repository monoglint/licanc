#pragma once

/*

INSTRUCTIONS FOR MODDING

Add a new ASTNodeType value. Create a new struct that inherits from Node.
Append the new struct type to the variant at the bottom of the file.

note: only have ast nodes inherit from decl, stmt, or expr if they can be guaranteed to be one. by that, imagine
a function argument ast node. its value can be any type of expression. only label an ast node as an expr if you want
it to be passable as a funciton argument

*/

#include <variant>
#include <type_traits>

#include "util/arena.hh"

#include "frontend/scan/token.hh"

#include "util/span.hh"
#include "util/safe_id.hh"

#include "manager/manager_types.hh"

namespace frontend::scan::ast {
    struct Node {
        Node(util::Span span)
            : span(std::move(span)) 
        {}

        util::Span span;
    };

    enum class DeclType {
        IMPORT,
        GLOBAL,
        FUNCTION,
        STRUCT,
        MODULE,
    };

    struct Decl : Node {
        Decl(util::Span span, DeclType decl_type)
            : Node(span), decl_type(decl_type) 
        {}

        DeclType decl_type;
    };

    enum class StmtType {
        RETURN,
        COMPOUND,
    };
        
    struct Stmt : Node {
        Stmt(util::Span span, StmtType stmt_type)
            : Node(span), stmt_type(stmt_type) 
        {}

        StmtType stmt_type;
    };

    enum class ExprType {
        IDENTIFIER,
        STRING_LITERAL,
        UNARY,
        BINARY,
        SCOPE_REFERENCE,
        MEMBER_REFERENCE,
        SCOPE_RESOLUTION,
        MEMBER_ACCESS,
        TERNARY,
        CALL,
        HEAP,
        TEMPLATE_inst,
    };

    struct Expr : Node {
        Expr(util::Span span, ExprType expr_type)
            : Node(span), expr_type(expr_type) 
        {}

        ExprType expr_type;
    };

    enum class TypenameType {
        NAMED,
        TEMPLATE_INSTANTIATED,
        QUALIFIED,
        REFERENCED,
        POINTERED,
        ARRAYED,
    };

    struct Typename : Node {
        Typename(util::Span span, TypenameType typename_type)
            : Node(span), typename_type(typename_type)
        {}

        TypenameType typename_type;
    };
    
    template <typename T>
    using Ptrs = std::vector<T*>;

    using NodePtrs = Ptrs<Node>;
    using DeclPtrs = Ptrs<Decl>;
    using StmtPtrs = Ptrs<Stmt>;
    using ExprPtrs = Ptrs<Expr>;

    //            ||||||
    //            ||||||
    // real nodes vvvvvv
    //
    //
    struct Root : Node {
        Root()
            : Node({}) 
        {}

        DeclPtrs decls;
    };

    // x
    struct Identifier : Expr {
        Identifier(util::Span span, manager::IdentifierId identifier_id)
            : Expr(std::move(span), ExprType::IDENTIFIER), identifier_id(identifier_id) 
        {}
        
        // reference to within manager::FrontendUnit
        manager::IdentifierId identifier_id;
    };

    // "hello world"
    struct StringLiteral : Expr {
        StringLiteral(util::Span span, manager::StringLiteralId string_literal_id)
            : Expr(std::move(span), ExprType::STRING_LITERAL), string_literal_id(string_literal_id) 
        {}

        manager::StringLiteralId string_literal_id;
    };

    // struct NumberLiteral : Node {
    //     NumberLiteral(util::Span span, manager::NumberLiteralId number_literal_id, token::TokenType suffix)
    //         : Node(std::move(span)), number_literal_id(number_literal_id), suffix(suffix) {}

    //     manager::NumberLiteralId number_literal_id;
    //     token::TokenType suffix;
    // };
    // ^^^^^^^^ update how number literals are stored during scan time before re-enabling this

    // -5
    struct UnaryExpr : Expr {
        UnaryExpr(util::Span span, Expr* operand, token::TokenType opr)
            : Expr(std::move(span), ExprType::UNARY), operand(operand), opr(opr) 
        {}

        Expr* operand;
        token::TokenType opr;
    };

    // 5 + 2
    struct BinaryExpr : Expr {
        BinaryExpr(util::Span span, Expr* operand0, Expr* operand1, token::TokenType opr)
            : Expr(std::move(span), ExprType::BINARY), operand0(operand0), operand1(operand1), opr(opr) 
        {}

        Expr* operand0;
        Expr* operand1;
        token::TokenType opr;
    };

    struct ScopeResolutionExpr;
    using ScopeReferenceVariant = std::variant<Identifier*, ScopeResolutionExpr*>;
    // a::b() || a()
    struct ScopeReferenceExpr : Expr {
        ScopeReferenceExpr(util::Span span, ScopeReferenceVariant reference)
            : Expr(std::move(span), ExprType::SCOPE_REFERENCE), reference(std::move(reference)) 
        {}

        ScopeReferenceVariant reference;
    };

    struct MemberAccessExpr;
    using MemberReferenceVariant = std::variant<Identifier*, MemberAccessExpr*>;
    struct MemberReferenceExpr : Expr {
        MemberReferenceExpr(util::Span span, MemberReferenceVariant reference)
            : Expr(std::move(span), ExprType::MEMBER_REFERENCE), reference(std::move(reference))
        {}

        MemberReferenceVariant reference;
    };

    // math::pi
    struct ScopeResolutionExpr : Expr {
        ScopeResolutionExpr(util::Span span, ScopeReferenceExpr* operand0, Identifier* operand1)
            : Expr(std::move(span), ExprType::SCOPE_RESOLUTION), operand0(operand0), operand1(operand1)
         {}

        ScopeReferenceExpr* operand0;
        Identifier* operand1;
    };
    
    // object.position.x
    struct MemberAccessExpr : Expr {
        MemberAccessExpr(util::Span span, MemberReferenceExpr* operand0, Identifier* operand1)
            : Expr(std::move(span), ExprType::MEMBER_ACCESS), operand0(operand0), operand1(operand1)
        {}

        MemberReferenceExpr* operand0;
        Identifier* operand1;
    };

    // if a > b { } else { } <--- not actual compound statement, braces are parser resolved syntactic sugar
    struct TernaryExpr : Expr {
        TernaryExpr(util::Span span, Expr* condition, Expr* consequent, Expr* alternate, token::TokenType opr)
            : Expr(std::move(span), ExprType::TERNARY), condition(condition), consequent(consequent), alternate(alternate), opr(opr) 
        {}

        Expr* condition;
        Expr* consequent;
        Expr* alternate;
        token::TokenType opr;
    };

    /*
    
    templated initializer in templated struct scenario
    util..reference_wrapper<int>..new<float>()

    */

    struct CallExpr : Expr {
        CallExpr(util::Span span, ScopeReferenceExpr* callee, ExprPtrs arguments = {})
            : Expr(std::move(span), ExprType::CALL), callee(callee), arguments(std::move(arguments))
        {}

        ScopeReferenceExpr* callee; // ScopeReference
        ExprPtrs arguments; // {Expr}
    };

    struct HeapExpr : Expr {
        HeapExpr(util::Span span, CallExpr* inst, std::optional<Expr*> address)
            : Expr(std::move(span), ExprType::HEAP), inst(inst), address(std::move(address))
        {}

        CallExpr* inst;
        std::optional<Expr*> address; // must semantically deduce to a pointer
    };

    struct TemplateArgument;
    struct TemplateInstExpr : Expr {
        TemplateInstExpr(util::Span span, ScopeReferenceExpr* target, std::vector<TemplateArgument*> template_arguments)
            : Expr(std::move(span), ExprType::TEMPLATE_inst), target(target), template_arguments(std::move(template_arguments))
        {}
        
        ScopeReferenceExpr* target;
        std::vector<TemplateArgument*> template_arguments;
    };

    struct NamedTypename : Typename {
        NamedTypename(util::Span span, ScopeReferenceExpr* base)
            : Typename(std::move(span), TypenameType::NAMED), base(base)
        {}

        ScopeReferenceExpr* base;
    };

    struct TemplateArgument;
    struct TemplateInstantiatedTypename : Typename {
        TemplateInstantiatedTypename(util::Span span, NamedTypename* base, std::vector<TemplateArgument*> arguments)
            : Typename(std::move(span), TypenameType::TEMPLATE_INSTANTIATED), base(base), arguments(std::move(arguments))
        {}

        NamedTypename* base;
        std::vector<TemplateArgument*> arguments;
    };

    enum class TypenameQualifier {
        CONST
    };

    struct QualifiedTypename : Typename {
        QualifiedTypename(util::Span span, Typename* base, TypenameQualifier qualifier)
            : Typename(std::move(span), TypenameType::QUALIFIED), base(base), qualifier(qualifier)
        {}

        Typename* base;
        TypenameQualifier qualifier;
    };

    struct ReferencedTypename : Typename {
        ReferencedTypename(util::Span span, Typename* base)
            : Typename(std::move(span), TypenameType::REFERENCED), base(base)
        {}

        Typename* base;
    };

    struct PointeredTypename : Typename {
        PointeredTypename(util::Span span, Typename* base)
            : Typename(std::move(span), TypenameType::POINTERED), base(base)
        {}

        Typename* base; // only limitation: no references
    };

    struct ArrayedTypename : Typename {
        ArrayedTypename(util::Span span, Typename* base)
            : Typename(std::move(span), TypenameType::ARRAYED), base(base)
        {}

        Typename* base;
    };

    /*

    ; file: "color.li"
    struct color {
        r: u8 = 0
        g: u8 = 0
        b: u8 = 0
    }

    ; file: "main.li"
    import "io.li" ; if io has its own module, then that module will be dumped into the global module here
    
    module art {
        import "color.li" ; imports can be oranized into local modules
    }

    art..color()
    
    */

    // import "math"
    struct ImportDecl : Decl {
        ImportDecl(util::Span span, StringLiteral* file_path, StringLiteral* absolute_file_path, bool is_path_valid)
            : Decl(std::move(span), DeclType::IMPORT), file_path(file_path), absolute_file_path(absolute_file_path), is_path_valid(is_path_valid) 
        {}

        StringLiteral* file_path;
        StringLiteral* absolute_file_path;
        bool is_path_valid;
        /* manager.cc */ manager::FileId resolved_file_id; // proper dependency connection is made here
    };

    // 
    struct GlobalDecl : Decl {
        GlobalDecl(util::Span span, Identifier* name, Typename* type, Expr* value)
            : Decl(std::move(span), DeclType::GLOBAL), name(name), type(type), value(value) 
        {}

        Identifier* name;
        Typename* type;
        Expr* value;
    };

    struct TypeNameTemplateParameter : Node {
        Identifier* name;
    };

    struct ValueTemplateParameter : Node {
        Identifier* name;
        Typename* type;
    };

    using TemplateParameterVariant = std::variant<TypeNameTemplateParameter*, ValueTemplateParameter*>;
    struct TemplateParameter : Node {
        TemplateParameter(util::Span span, TemplateParameterVariant value)
            : Node(std::move(span)), value(std::move(value)) 
        {}

        TemplateParameterVariant value;
    };

    using TemplateArgumentValueVariant = std::variant<Typename*, Expr*>;
    struct TemplateArgument : Node {
        TemplateArgument(util::Span span, TemplateArgumentValueVariant value)
            : Node(std::move(span)), value(std::move(value)) 
        {}

        // note: if value is a Expr, then the value of the template argument must be proven to be a compile time constant
        TemplateArgumentValueVariant value;
    };

    struct FunctionParameter : Node {
        FunctionParameter(util::Span span, Identifier* name, Typename* type)
            : Node(std::move(span)), name(name), type(type) 
        {}

        Identifier* name;
        Typename* type;
    };
    
    struct Function : Node {
        Function(util::Span span, std::vector<FunctionParameter*> parameters, Stmt* body, Typename* return_type)
            : Node(std::move(span)), parameters(std::move(parameters)), body(body), return_type(return_type) 
        {}

        std::vector<FunctionParameter*> parameters;
        Stmt* body;
        Typename* return_type;
    };

    struct FunctionTemplate : Node {
        FunctionTemplate(util::Span span, Function* base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(span)), base(base), template_parameters(std::move(template_parameters)) 
        {}

        Function* base;
        std::vector<TemplateParameter*> template_parameters; // {TemplateParameter}
    };
    
    struct FunctionDecl : Decl {
        FunctionDecl(util::Span span, FunctionTemplate* function_template, Identifier* name)
            : Decl(std::move(span), DeclType::FUNCTION), function_template(function_template), name(name) 
            {}

        FunctionTemplate* function_template;
        Identifier* name;
    };

    struct Initializer : Node {
        Initializer(util::Span span, Identifier* name, Function* function)
            : Node(std::move(span)), name(name), function(function) 
        {}

        Identifier* name; // Identifier
        Function* function; // NO DEPENDENT TYPES
    };

    struct Finalizer : Node {
        Finalizer(util::Span span, Function* function)
            : Node(std::move(span)), function(function) 
        {}

        Function* function; // NO DEPENDENT TYPES
    };

    struct Method : Node {
        Method(util::Span span, token::TokenType access_specifier, Identifier* name, FunctionTemplate* function_template)
            : Node(std::move(span)), access_specifier(access_specifier), name(name), function_template(function_template) 
        {}

        token::TokenType access_specifier;
        Identifier* name;
        FunctionTemplate* function_template;
    };

    struct Property : Node {
        Property(util::Span span, token::TokenType access_specifier, Identifier* name, Typename* type)
            : Node(std::move(span)), access_specifier(access_specifier), name(name), type(type) 
        {}

        token::TokenType access_specifier;
        Identifier* name;
        Typename* type;
    };

    struct Struct : Node {
        Struct(util::Span span, std::vector<Method*> methods, std::vector<Property*> properties, std::vector<Initializer*> initializers, Finalizer* finalizer)
            : Node(std::move(span)), methods(std::move(methods)), properties(std::move(properties)), initializers(std::move(initializers)), finalizer(finalizer) 
        {}

        std::vector<Method*> methods;
        std::vector<Property*> properties;
        std::vector<Initializer*> initializers;
        Finalizer* finalizer; // Finalizer?
    };

    struct StructTemplate : Node {
        StructTemplate(util::Span span, Struct* base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(span)), base(base), template_parameters(std::move(template_parameters)) 
        {}

        Struct* base;
        std::vector<TemplateParameter*> template_parameters;
    };

    struct StructDecl : Decl {
        StructDecl(util::Span span, StructTemplate* struct_template, Identifier* name)
            : Decl(std::move(span), DeclType::STRUCT), struct_template(struct_template), name(name) 
        {}

        StructTemplate* struct_template;
        Identifier* name;
    };

    // struct TypeAliasTemplate : Node {
    //     TypeAliasTemplate(util::Span span, Type* type, std::vector<Type*> insts, std::vector<TemplateParameter*> template_parameters)
    //         : Node(std::move(span)), type(type), insts(std::move(insts)), template_parameters(std::move(template_parameters)) {}

    //     Type* type;
    //     std::vector<Type*> insts;
    //     std::vector<TemplateParameter*> template_parameters;
    // };

    // struct TypeAliasDecl : Item {
    //     TypeAliasDecl(util::Span span, TypeAliasTemplate* type_alias_template, Identifier* name)
    //         : Item(std::move(span)), type_alias_template(type_alias_template), name(name) {}

    //     TypeAliasTemplate* type_alias_template;
    //     Identifier* name;
    // };

    struct ModuleDecl : Decl {
        ModuleDecl(util::Span span, Identifier* name, DeclPtrs decls)
            : Decl(std::move(span), DeclType::MODULE), name(name), decls(std::move(decls)) 
        {}
        
        Identifier* name; // Identifier
        DeclPtrs decls; // {Item}

        // ^^^ in the symbol table, a module declaration's decls are split into decls and references.
    };

    // -- STATEMENTS

    struct ReturnStmt : Stmt {
        ReturnStmt(util::Span span, Expr* value)
            : Stmt(std::move(span), StmtType::RETURN), value(value) 
        {}

        Expr* value;
    };

    struct CompoundStmt : Stmt {
        CompoundStmt(util::Span span, StmtPtrs stmts)
            : Stmt(std::move(span), StmtType::COMPOUND), stmts(std::move(stmts)) 
        {}

        StmtPtrs stmts;
    };

    //
    //
    //

    class AST {
    public:
        AST() { init(); }

        // if an import node is allocated into the arena, its important to add it to the "imports" vector

        Root* root_ptr;
        std::vector<ImportDecl*> import_nodes;
        
        template <std::derived_from<Node> T, typename... ARGS>
        T* emplace(ARGS... args);

        template <std::derived_from<Node> T>
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