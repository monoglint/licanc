#pragma once

/*

*/

#include <variant>

#include "util/arena.hh"

#include "frontend/scan/tok.hh"

#include "util/span.hh"

#include "manager/manager_types.hh"

namespace frontend::scan::ast {
    struct Node {
        Node(util::Span span)
            : span(std::move(span))
        {}

        util::Span span;
    };

    enum class DeclKind {
        IMPORT,
        GLOBAL,
        FUNCTION,
        STRUCT,
        MODULE,
        TYPE_ALIAS,
    };

    struct Visibility;
    struct Decl : Node {
        Decl(util::Span span, DeclKind decl_kind, Visibility* visibility)
            : Node(span), decl_kind(decl_kind), visibility(visibility)
        {}

        DeclKind decl_kind;
        Visibility* visibility;
    };

    enum class StmtKind {
        RETURN,
        COMPOUND,
    };
        
    struct Stmt : Node {
        Stmt(util::Span span, StmtKind stmt_kind)
            : Node(span), stmt_kind(stmt_kind)
        {}

        StmtKind stmt_kind;
    };

    enum class ExprKind {
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
        Expr(util::Span span, ExprKind expr_kind)
            : Node(span), expr_kind(expr_kind)
        {}

        ExprKind expr_kind;
    };

    enum class TypeKind {
        NAMED,
        TEMPLATE_INSTANTIATED,
        QUALIFIED,
        REFERENCED,
        POINTERED,
        ARRAYED,
    };

    struct Type : Node {
        Type(util::Span span, TypeKind type_kind)
            : Node(span), type_kind(type_kind)
        {}

        TypeKind type_kind;
    };

    //            ||||||
    //            ||||||
    // real nodes vvvvvv
    //
    //
    struct Root : Node {
        Root()
            : Node({})
        {}

        std::vector<Decl*> decls;
    };

    struct Visibility : Node {
        Visibility(util::Span span)
            : Node(std::move(span))
        {}

    };

    // x
    struct Identifier : Expr {
        Identifier(util::Span span, manager::IdentifierId identifier_id)
            : Expr(std::move(span), ExprKind::IDENTIFIER), identifier_id(identifier_id)
        {}
        
        // reference to within manager::FrontendUnit
        manager::IdentifierId identifier_id;
    };

    // "hello world"
    struct StringLiteral : Expr {
        StringLiteral(util::Span span, manager::StringLiteralId string_literal_id)
            : Expr(std::move(span), ExprKind::STRING_LITERAL), string_literal_id(string_literal_id)
        {}

        manager::StringLiteralId string_literal_id;
    };

    // struct NumberLiteral : Node {
    //     NumberLiteral(util::Span span, manager::NumberLiteralId number_literal_id, tok::TokenType suffix)
    //         : Node(std::move(span)), number_literal_id(number_literal_id), suffix(suffix) {}

    //     manager::NumberLiteralId number_literal_id;
    //     tok::TokenType suffix;
    // };
    // ^^^^^^^^ update how number literals are stored during scan time before re-enabling this

    // -5
    struct UnaryExpr : Expr {
        UnaryExpr(util::Span span, Expr* operand, tok::OperatorKind opr)
            : Expr(std::move(span), ExprKind::UNARY), operand(operand), opr(opr)
        {}

        Expr* operand;
        tok::OperatorKind opr;
    };

    // 5 + 2
    struct BinaryExpr : Expr {
        BinaryExpr(util::Span span, Expr* operand0, Expr* operand1, tok::OperatorKind opr)
            : Expr(std::move(span), ExprKind::BINARY), operand0(operand0), operand1(operand1), opr(opr)
        {}

        Expr* operand0;
        Expr* operand1;
        tok::OperatorKind opr;
    };

    struct ScopeResolutionExpr;
    using ScopeReferenceVariant = std::variant<Identifier*, ScopeResolutionExpr*>;
    // a::b() || a()
    struct ScopeReferenceExpr : Expr {
        ScopeReferenceExpr(util::Span span, ScopeReferenceVariant reference)
            : Expr(std::move(span), ExprKind::SCOPE_REFERENCE), reference(std::move(reference))
        {}

        ScopeReferenceVariant reference;
    };

    struct MemberAccessExpr;
    using MemberReferenceVariant = std::variant<Identifier*, MemberAccessExpr*>;
    struct MemberReferenceExpr : Expr {
        MemberReferenceExpr(util::Span span, MemberReferenceVariant reference)
            : Expr(std::move(span), ExprKind::MEMBER_REFERENCE), reference(std::move(reference))
        {}

        MemberReferenceVariant reference;
    };

    // math::pi
    struct ScopeResolutionExpr : Expr {
        ScopeResolutionExpr(util::Span span, ScopeReferenceExpr* operand0, Identifier* operand1)
            : Expr(std::move(span), ExprKind::SCOPE_RESOLUTION), operand0(operand0), operand1(operand1)
         {}

        ScopeReferenceExpr* operand0;
        Identifier* operand1;
    };
    
    // object.position.x
    struct MemberAccessExpr : Expr {
        MemberAccessExpr(util::Span span, MemberReferenceExpr* operand0, Identifier* operand1)
            : Expr(std::move(span), ExprKind::MEMBER_ACCESS), operand0(operand0), operand1(operand1)
        {}

        MemberReferenceExpr* operand0;
        Identifier* operand1;
    };

    // if a > b { } else { } <--- not actual compound statement, braces are parser resolved syntactic sugar
    struct TernaryExpr : Expr {
        TernaryExpr(util::Span span, Expr* condition, Expr* consequent, Expr* alternate)
            : Expr(std::move(span), ExprKind::TERNARY), condition(condition), consequent(consequent), alternate(alternate)
        {}

        Expr* condition;
        Expr* consequent;
        Expr* alternate;
    };

    /*
    
    templated initializer in templated struct scenario
    util..reference_wrapper<int>..new<float>()

    */

    struct CallExpr : Expr {
        CallExpr(util::Span span, ScopeReferenceExpr* callee, std::vector<Expr*> arguments)
            : Expr(std::move(span), ExprKind::CALL), callee(callee), arguments(std::move(arguments))
        {}

        ScopeReferenceExpr* callee; // ScopeReference
        std::vector<Expr*> arguments; // {Expr}
    };

    struct HeapExpr : Expr {
        HeapExpr(util::Span span, CallExpr* inst, std::optional<Expr*> address)
            : Expr(std::move(span), ExprKind::HEAP), inst(inst), address(std::move(address))
        {}

        CallExpr* inst;
        std::optional<Expr*> address; // must semantically deduce to a pointer
    };

    struct TemplateArgument;
    struct TemplateInstExpr : Expr {
        TemplateInstExpr(util::Span span, ScopeReferenceExpr* target, std::vector<TemplateArgument*> template_arguments)
            : Expr(std::move(span), ExprKind::TEMPLATE_inst), target(target), template_arguments(std::move(template_arguments))
        {}
        
        ScopeReferenceExpr* target;
        std::vector<TemplateArgument*> template_arguments;
    };

    struct NamedType : Type {
        NamedType(util::Span span, ScopeReferenceExpr* base)
            : Type(std::move(span), TypeKind::NAMED), base(base)
        {}

        ScopeReferenceExpr* base;
    };

    struct TemplateArgument;
    struct TemplateInstantiatedType : Type {
        TemplateInstantiatedType(util::Span span, NamedType* base, std::vector<TemplateArgument*> arguments)
            : Type(std::move(span), TypeKind::TEMPLATE_INSTANTIATED), base(base), arguments(std::move(arguments))
        {}

        NamedType* base;
        std::vector<TemplateArgument*> arguments;
    };

    enum class TypeQualifier {
        CONST
    };

    enum class ResolvedTypeQualifier {
        MUT
    };

    struct QualifiedType : Type {
        QualifiedType(util::Span span, Type* base, TypeQualifier qualifier)
            : Type(std::move(span), TypeKind::QUALIFIED), base(base), qualifier(qualifier)
        {}

        Type* base;
        TypeQualifier qualifier;
    };

    struct ReferencedType : Type {
        ReferencedType(util::Span span, Type* base)
            : Type(std::move(span), TypeKind::REFERENCED), base(base)
        {}

        Type* base;
    };

    struct PointeredType : Type {
        PointeredType(util::Span span, Type* base)
            : Type(std::move(span), TypeKind::POINTERED), base(base)
        {}

        Type* base; // only limitation: no references
    };

    struct ArrayedType : Type {
        ArrayedType(util::Span span, Type* base)
            : Type(std::move(span), TypeKind::ARRAYED), base(base)
        {}

        Type* base;
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
        ImportDecl(util::Span span, Visibility* visibility, StringLiteral* file_path, StringLiteral* absolute_file_path, bool is_path_valid)
            : Decl(std::move(span), DeclKind::IMPORT, visibility), file_path(file_path), absolute_file_path(absolute_file_path), is_path_valid(is_path_valid)
        {}

        StringLiteral* file_path;
        StringLiteral* absolute_file_path;
        bool is_path_valid;
        /* manager.cc */ manager::FileId resolved_file_id; // proper dependency connection is made here
    };

    // 
    struct GlobalDecl : Decl {
        GlobalDecl(util::Span span, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, Identifier* name, Type* type, Expr* value)
            : Decl(std::move(span), DeclKind::GLOBAL, visibility), storage_specifiers(storage_specifiers), name(name), type(type), value(value)
        {}

        tok::StorageSpecifierFlags storage_specifiers;
        Identifier* name;
        Type* type;
        Expr* value;
    };

    struct TypeNameTemplateParameter : Node {
        Identifier* name;
    };

    struct ValueTemplateParameter : Node {
        Identifier* name;
        Type* type;
    };

    using TemplateParameterVariant = std::variant<TypeNameTemplateParameter*, ValueTemplateParameter*>;
    struct TemplateParameter : Node {
        TemplateParameter(util::Span span, TemplateParameterVariant value)
            : Node(std::move(span)), value(std::move(value))
        {}

        TemplateParameterVariant value;
    };

    using TemplateArgumentValueVariant = std::variant<Type*, Expr*>;
    struct TemplateArgument : Node {
        TemplateArgument(util::Span span, TemplateArgumentValueVariant value)
            : Node(std::move(span)), value(std::move(value))
        {}

        // note: if value is a Expr, then the value of the template argument must be proven to be a compile time constant
        TemplateArgumentValueVariant value;
    };

    struct FunctionParameter : Node {
        FunctionParameter(util::Span span, Identifier* name, Type* type)
            : Node(std::move(span)), name(name), type(type)
        {}

        Identifier* name;
        Type* type;
    };
    
    struct Function : Node {
        Function(util::Span span, std::vector<FunctionParameter*> parameters, Stmt* body, Type* return_type)
            : Node(std::move(span)), parameters(std::move(parameters)), body(body), return_type(return_type)
        {}

        std::vector<FunctionParameter*> parameters;
        Stmt* body;
        Type* return_type;
    };

    struct FunctionTemplate : Node {
        FunctionTemplate(util::Span span, Function* base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(span)), base(base), template_parameters(std::move(template_parameters))
        {}

        Function* base;
        std::vector<TemplateParameter*> template_parameters; // {TemplateParameter}
    };
    
    struct FunctionDecl : Decl {
        FunctionDecl(util::Span span, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, FunctionTemplate* function_template, Identifier* name)
            : Decl(std::move(span), DeclKind::FUNCTION, visibility), storage_specifiers(storage_specifiers), function_template(function_template), name(name)
            {}

        tok::StorageSpecifierFlags storage_specifiers;
        FunctionTemplate* function_template;
        Identifier* name;
    };

    struct Finalizer : Node {
        Finalizer(util::Span span, Function* function)
            : Node(std::move(span)), function(function)
        {}

        Function* function; // COMPLETELY CONCRETE
    };

    struct Method : Node {
        Method(util::Span span, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, Identifier* name, FunctionTemplate* function_template)
            : Node(std::move(span)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), function_template(function_template)
        {}

        Visibility* visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        Identifier* name;
        FunctionTemplate* function_template;
    };

    struct Property : Node {
        Property(util::Span span, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, Identifier* name, Type* type)
            : Node(std::move(span)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), type(type)
        {}

        Visibility* visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        Identifier* name;
        Type* type;
    };

    struct Struct : Node {
        Struct(util::Span span, std::vector<Method*> methods, std::vector<Property*> properties, Finalizer* finalizer)
            : Node(std::move(span)), methods(std::move(methods)), properties(std::move(properties)), finalizer(finalizer)
        {}

        std::vector<Method*> methods;
        std::vector<Property*> properties;
        // TODO: additional optional declarations need to be added like static variables, imports, etc.
        
        Finalizer* finalizer; // WHEN TRAITS ADDED, DONT HAVE DESTRUCTORS HERE!!
    };

    struct StructTemplate : Node {
        StructTemplate(util::Span span, Struct* base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(span)), base(base), template_parameters(std::move(template_parameters))
        {}

        Struct* base;
        std::vector<TemplateParameter*> template_parameters;
    };

    struct StructDecl : Decl {
        StructDecl(util::Span span, Visibility* visibility, StructTemplate* struct_template, Identifier* name)
            : Decl(std::move(span), DeclKind::STRUCT, visibility), struct_template(struct_template), name(name)
        {}

        StructTemplate* struct_template;
        Identifier* name;
    };

    struct TypeAliasTemplate : Node {
        TypeAliasTemplate(util::Span span, Type* type, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(span)), type(type), template_parameters(std::move(template_parameters)) {}

        Type* type;
        std::vector<TemplateParameter*> template_parameters;
    };

    struct TypeAliasDecl : Decl {
        TypeAliasDecl(util::Span span, Visibility* visibility, TypeAliasTemplate* type_alias_template, Identifier* name)
            : Decl(std::move(span), DeclKind::TYPE_ALIAS, visibility), type_alias_template(type_alias_template), name(name) {}

        TypeAliasTemplate* type_alias_template;
        Identifier* name;
    };

    struct ModuleDecl : Decl {
        ModuleDecl(util::Span span, Visibility* visibility, Identifier* name, std::vector<Decl*> decls)
            : Decl(std::move(span), DeclKind::MODULE, visibility), name(name), decls(std::move(decls))
        {}
        
        Identifier* name;
        std::vector<Decl*> decls;

        // ^^^ in the symbol table, a module declaration's decls are split into decls and references.
    };

    // -- STATEMENTS

    struct ReturnStmt : Stmt {
        ReturnStmt(util::Span span, Expr* value)
            : Stmt(std::move(span), StmtKind::RETURN), value(value)
        {}

        Expr* value;
    };

    struct CompoundStmt : Stmt {
        CompoundStmt(util::Span span, std::vector<Stmt*> stmts)
            : Stmt(std::move(span), StmtKind::COMPOUND), stmts(std::move(stmts))
        {}

        std::vector<Stmt*> stmts;
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
        T* push(T node);

        void clear() {
            arena.clear();
            init();
        }

    private:
        // EXTERNAL CALL OF ::clear() IS UNSAFE
        util::Arena<> arena;

        void init() {
            std::ignore = arena.try_emplace<Root>();
        }
    };
}