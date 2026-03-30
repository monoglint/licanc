module;

#include <variant>
#include <vector>

export module frontend.ast:ast_nodes;

import driver_base;

import frontend.tok;
import util;

export namespace frontend::ast {
    struct Node {
        Node(util::Span span)
            : span(std::move(span))
        {}

        util::Span span;
    };

    enum class DeclKind {
        GLOBAL,
        FUNCTION,
        STRUCT,
        MODULE,
        TYPE_ALIAS,
    };

    struct Visibility;
    struct Decl : Node {
        Decl(util::Span span, DeclKind decl_kind, util::FirmPtr<Visibility> visibility)
            : Node(span), decl_kind(decl_kind), visibility(visibility)
        {}

        DeclKind decl_kind;
        util::FirmPtr<Visibility> visibility;
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
    };

    struct Type : Node {
        Type(util::Span span, TypeKind type_kind)
            : Node(span), type_kind(type_kind)
        {}

        TypeKind type_kind;
    };

    //
    //
    //

    struct Visibility : Node {
        Visibility(util::Span span)
            : Node(std::move(span))
        {}

        // the target declaration and any symbols inside of it will have access.
        util::OptPtr<Decl> accessor;
    };

    struct Root : Node {
        Root()
            : Node({})
        {}

        std::vector<util::FirmPtr<Decl>> decls;
    };

    // x
    struct Identifier : Expr {
        Identifier(util::Span span, driver_base::IdentifierIndex identifier_index)
            : Expr(std::move(span), ExprKind::IDENTIFIER), identifier_index(identifier_index)
        {}
        
        driver_base::IdentifierIndex identifier_index;
    };

    // "hello world"
    struct StringLiteral : Expr {
        StringLiteral(util::Span span, driver_base::StringLiteralIndex string_literal_index)
            : Expr(std::move(span), ExprKind::STRING_LITERAL), string_literal_index(string_literal_index)
        {}

        driver_base::StringLiteralIndex string_literal_index;
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
        UnaryExpr(util::Span span, util::FirmPtr<Expr> operand, tok::OperatorKind opr)
            : Expr(std::move(span), ExprKind::UNARY), operand(operand), opr(opr)
        {}

        util::FirmPtr<Expr> operand;
        tok::OperatorKind opr;
    };

    // 5 + 2
    struct BinaryExpr : Expr {
        BinaryExpr(util::Span span, util::FirmPtr<Expr> operand0, util::FirmPtr<Expr> operand1, tok::OperatorKind opr)
            : Expr(std::move(span), ExprKind::BINARY), operand0(operand0), operand1(operand1), opr(opr)
        {}

        util::FirmPtr<Expr> operand0;
        util::FirmPtr<Expr> operand1;
        tok::OperatorKind opr;
    };

    struct ScopeResolutionExpr;
    // a::b() || a()
    struct ScopeReferenceExpr : Expr {
        using ReferenceVariant = std::variant<util::FirmPtr<Identifier>, util::FirmPtr<ScopeResolutionExpr>>;

        ScopeReferenceExpr(util::Span span, ReferenceVariant reference)
            : Expr(std::move(span), ExprKind::SCOPE_REFERENCE), reference(std::move(reference))
        {}

        ReferenceVariant reference;
    };

    struct MemberAccessExpr;
    struct MemberReferenceExpr : Expr {
        using ReferenceVariant = std::variant<util::FirmPtr<Identifier>, util::FirmPtr<MemberAccessExpr>>;

        MemberReferenceExpr(util::Span span, ReferenceVariant reference)
            : Expr(std::move(span), ExprKind::MEMBER_REFERENCE), reference(std::move(reference))
        {}

        ReferenceVariant reference;
    };

    // math::pi
    struct ScopeResolutionExpr : Expr {
        ScopeResolutionExpr(util::Span span, util::FirmPtr<ScopeReferenceExpr> operand0, util::FirmPtr<Identifier> operand1)
            : Expr(std::move(span), ExprKind::SCOPE_RESOLUTION), operand0(operand0), operand1(operand1)
        {}

        util::FirmPtr<ScopeReferenceExpr> operand0;
        util::FirmPtr<Identifier> operand1;
    };
    
    // object.position.x
    struct MemberAccessExpr : Expr {
        MemberAccessExpr(util::Span span, util::FirmPtr<MemberReferenceExpr> operand0, util::FirmPtr<Identifier> operand1)
            : Expr(std::move(span), ExprKind::MEMBER_ACCESS), operand0(operand0), operand1(operand1)
        {}

        util::FirmPtr<MemberReferenceExpr> operand0;
        util::FirmPtr<Identifier> operand1;
    };

    // if a > b { } else { } <--- not actual compound statement, braces are parser resolved syntactic sugar
    struct TernaryExpr : Expr {
        TernaryExpr(util::Span span, util::FirmPtr<Expr> condition, util::FirmPtr<Expr> consequent, util::FirmPtr<Expr> alternate)
            : Expr(std::move(span), ExprKind::TERNARY), condition(condition), consequent(consequent), alternate(alternate)
        {}

        util::FirmPtr<Expr> condition;
        util::FirmPtr<Expr> consequent;
        util::FirmPtr<Expr> alternate;
    };

    /*
    
    templated initializer in templated struct scenario
    util..reference_wrapper<int>..new<float>()

    */

    struct CallExpr : Expr {
        CallExpr(util::Span span, util::FirmPtr<ScopeReferenceExpr> callee, std::vector<util::FirmPtr<Expr>> arguments)
            : Expr(std::move(span), ExprKind::CALL), callee(callee), arguments(std::move(arguments))
        {}

        util::FirmPtr<ScopeReferenceExpr> callee; // ScopeReference
        std::vector<util::FirmPtr<Expr>> arguments; // {Expr}
    };

    struct HeapExpr : Expr {
        HeapExpr(util::Span span, util::FirmPtr<CallExpr> inst, util::OptPtr<Expr> address)
            : Expr(std::move(span), ExprKind::HEAP), inst(inst), address(std::move(address))
        {}

        util::FirmPtr<CallExpr> inst;
        util::OptPtr<Expr> address; // must semantically deduce to a pointer
    };

    struct TemplateArgument;
    struct TemplateInstExpr : Expr {
        TemplateInstExpr(util::Span span, util::FirmPtr<ScopeReferenceExpr> target, std::vector<TemplateArgument*> template_arguments)
            : Expr(std::move(span), ExprKind::TEMPLATE_inst), target(target), template_arguments(std::move(template_arguments))
        {}
        
        util::FirmPtr<ScopeReferenceExpr> target;
        std::vector<TemplateArgument*> template_arguments;
    };

    struct NamedType : Type {
        NamedType(util::Span span, util::FirmPtr<ScopeReferenceExpr> base)
            : Type(std::move(span), TypeKind::NAMED), base(base)
        {}

        util::FirmPtr<ScopeReferenceExpr> base;
    };

    struct TemplateArgument;
    struct TemplateInstantiatedType : Type {
        TemplateInstantiatedType(util::Span span, util::FirmPtr<NamedType> base, std::vector<TemplateArgument*> arguments)
            : Type(std::move(span), TypeKind::TEMPLATE_INSTANTIATED), base(base), arguments(std::move(arguments))
        {}

        util::FirmPtr<NamedType> base;
        std::vector<TemplateArgument*> arguments;
    };

    enum class TypeQualifier {
        MUT
    };

    struct QualifiedType : Type {
        QualifiedType(util::Span span, util::FirmPtr<Type> base, TypeQualifier qualifier)
            : Type(std::move(span), TypeKind::QUALIFIED), base(base), qualifier(qualifier)
        {}

        util::FirmPtr<Type> base;
        TypeQualifier qualifier;
    };

    struct GlobalDecl : Decl {
        GlobalDecl(util::Span span, util::FirmPtr<Visibility> visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type, util::FirmPtr<Expr> value)
            : Decl(std::move(span), DeclKind::GLOBAL, visibility), storage_specifiers(storage_specifiers), name(name), type(type), value(value)
        {}

        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
        util::FirmPtr<Expr> value;
    };

    struct TypeTemplateParameter : Node {
        util::FirmPtr<Identifier> name;
    };

    struct ValueTemplateParameter : Node {
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };

    struct TemplateParameter : Node {
        using ValueVariant = std::variant<TypeTemplateParameter*, ValueTemplateParameter*>;

        TemplateParameter(util::Span span, ValueVariant value)
            : Node(std::move(span)), value(std::move(value))
        {}

        ValueVariant value;
    };

    struct TypeTemplateArgument : Node {
        TypeTemplateArgument(util::Span span, util::FirmPtr<Type> value)
            : Node(std::move(span)), value(value)
        {}

        util::FirmPtr<Type> value; // proven as compile-time constant by semantic analyzer
    };

    struct ValueTemplateArgument : Node {
        ValueTemplateArgument(util::Span span, util::FirmPtr<Expr> value)
            : Node(std::move(span)), value(value)
        {}
        
        util::FirmPtr<Expr> value;
    };

    struct TemplateArgument : Node {
        using ValueVariant = std::variant<TypeTemplateArgument*, ValueTemplateArgument*>;
        TemplateArgument(util::Span span, ValueVariant value)
            : Node(std::move(span)), value(std::move(value))
        {}

        ValueVariant value;
    };

    struct FunctionParameter : Node {
        FunctionParameter(util::Span span, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type)
            : Node(std::move(span)), name(name), type(type)
        {}

        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };
    
    struct Function : Node {
        Function(util::Span span, std::vector<FunctionParameter*> parameters, util::FirmPtr<Stmt> body, util::FirmPtr<Type> return_type)
            : Node(std::move(span)), parameters(std::move(parameters)), body(body), return_type(return_type)
        {}

        std::vector<FunctionParameter*> parameters;
        util::FirmPtr<Stmt> body;
        util::FirmPtr<Type> return_type;
    };

    struct FunctionTemplate : Node {
        FunctionTemplate(util::Span span, util::FirmPtr<Function> base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(span)), base(base), template_parameters(std::move(template_parameters))
        {}

        util::FirmPtr<Function> base;
        std::vector<TemplateParameter*> template_parameters; // {TemplateParameter}
    };
    
    struct FunctionDecl : Decl {
        FunctionDecl(util::Span span, util::FirmPtr<Visibility> visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<FunctionTemplate> function_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(span), DeclKind::FUNCTION, visibility), storage_specifiers(storage_specifiers), function_template(function_template), name(name)
            {}

        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<FunctionTemplate> function_template;
        util::FirmPtr<Identifier> name;
    };

    struct Finalizer : Node {
        Finalizer(util::Span span, Function* function)
            : Node(std::move(span)), function(function)
        {}

        Function* function; // COMPLETELY CONCRETE
    };

    struct Method : Node {
        Method(util::Span span, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, FunctionTemplate* function_template)
            : Node(std::move(span)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), function_template(function_template)
        {}

        Visibility* visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        FunctionTemplate* function_template;
    };

    struct Property : Node {
        Property(util::Span span, util::FirmPtr<Visibility> visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type)
            : Node(std::move(span)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), type(type)
        {}

        util::FirmPtr<Visibility> visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
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
        StructDecl(util::Span span, util::FirmPtr<Visibility> visibility, util::FirmPtr<StructTemplate> struct_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(span), DeclKind::STRUCT, visibility), struct_template(struct_template), name(name)
        {}

        util::FirmPtr<StructTemplate> struct_template;
        util::FirmPtr<Identifier> name;
    };

    struct TypeAliasTemplate : Node {
        TypeAliasTemplate(util::Span span, util::FirmPtr<Type> type, std::vector<util::FirmPtr<TemplateParameter>> template_parameters)
            : Node(std::move(span)), type(type), template_parameters(std::move(template_parameters)) {}

        util::FirmPtr<Type> type;
        std::vector<util::FirmPtr<TemplateParameter>> template_parameters;
    };

    struct TypeAliasDecl : Decl {
        TypeAliasDecl(util::Span span, util::FirmPtr<Visibility> visibility, util::FirmPtr<TypeAliasTemplate> type_alias_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(span), DeclKind::TYPE_ALIAS, visibility), type_alias_template(type_alias_template), name(name) {}

        util::FirmPtr<TypeAliasTemplate> type_alias_template;
        util::FirmPtr<Identifier> name;
    };

    struct ModuleDecl : Decl {
        ModuleDecl(util::Span span, util::FirmPtr<Visibility> visibility, util::FirmPtr<Identifier> name, std::vector<util::FirmPtr<Decl>> decls)
            : Decl(std::move(span), DeclKind::MODULE, visibility), name(name), decls(std::move(decls))
        {}
        
        util::FirmPtr<Identifier> name;
        std::vector<util::FirmPtr<Decl>> decls;

        // ^^^ in the symbol table, a module declaration's decls are split into decls and references.
    };

    // -- STATEMENTS

    struct ReturnStmt : Stmt {
        ReturnStmt(util::Span span, util::FirmPtr<Expr> value)
            : Stmt(std::move(span), StmtKind::RETURN), value(value)
        {}

        util::FirmPtr<Expr> value;
    };

    struct CompoundStmt : Stmt {
        CompoundStmt(util::Span span, std::vector<Stmt*> stmts)
            : Stmt(std::move(span), StmtKind::COMPOUND), stmts(std::move(stmts))
        {}

        std::vector<Stmt*> stmts;
    };
}