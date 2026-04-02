/*

This parition contains all of Lican's AST nodes.

@monoglint
Last updated: 31 March 2026

*/

/*

Examples:

=== 1 === - 31 March 2026

    var my_car = vehicles::Car[Deisel]

    TemplateInstExpr
        target: ScopeResolutionExpr
            operand0:
                DeclReferenceExpr
                    reference: Identifier: "vehicles"
            operand1:
                Identifier: "Car"
        template_args:
            TypeTemplateArg
                value: NamedType: "Deisel"


    notes:
        - ScopeResolution can potentially have operand0 be either TemplateInstExpr or a DeclReferenceExpr.
            I did not want to couple template instantiation inside of DeclReferenceExpr which is why the variant is needed.
*/

module;

#include <variant>
#include <vector>

export module frontend.ast:ast_nodes;

import :ast_pools;

import frontend.tok;
import util;

export namespace frontend::ast {
    using NodeId = util::SafeId<class NodeIdTag>;

    struct NodeInitParams {
        NodeInitParams(util::Span span, NodeId id)
            : span(std::move(span)), id(std::move(id))
        {}

        util::Span span;
        NodeId id;
    };
    
    struct Node {
        Node(NodeInitParams init_params)
            : span(std::move(init_params.span)), id(std::move(init_params.id))
        {}

        util::Span span;
        NodeId id;
    };

    enum class DeclKind {
        GLOBAL,
        FUNCTION,
        STRUCT,
        MODULE,
        TYPE_ALIAS,
    };

    struct Visibility;

    // i will admit, there are some unnecessary moves in here, but i want everything consistent when it comes to
    // presenting these init_param structs as an explicit transferer of data
    struct DeclInitParams {
        DeclInitParams(NodeInitParams node_init_params, util::FirmPtr<Visibility> visibility)
            : node_init_params(std::move(node_init_params)), visibility(std::move(visibility))
        {}

        NodeInitParams node_init_params;
        util::FirmPtr<Visibility> visibility;
    };

    struct Decl : Node {
        Decl(DeclInitParams init_params, DeclKind decl_kind)
            : Node(std::move(init_params.node_init_params)), decl_kind(decl_kind), visibility(std::move(init_params.visibility))
        {}

        DeclKind decl_kind;
        util::FirmPtr<Visibility> visibility;
    };

    enum class StmtKind {
        RETURN,
        COMPOUND,
    };
        
    struct Stmt : Node {
        Stmt(NodeInitParams init_params, StmtKind stmt_kind)
            : Node(std::move(init_params)), stmt_kind(stmt_kind)
        {}

        StmtKind stmt_kind;
    };

    enum class ExprKind {
        IDENTIFIER,
        STRING_LITERAL,
        UNARY,
        BINARY,
        DECL_REFERENCE,
        MEMBER_REFERENCE,
        SCOPE_RESOLUTION,
        MEMBER_ACCESS,
        TERNARY,
        CALL,
        HEAP,
        TEMPLATE_inst,
    };

    struct ExprInitParams {
        ExprInitParams(NodeInitParams node_init_params, ExprKind expr_kind)
            : node_init_params(std::move(node_init_params)), expr_kind(std::move(expr_kind))
        {}

        NodeInitParams node_init_params;
        ExprKind expr_kind;
    };

    struct Expr : Node {
        Expr(ExprInitParams init_params, ExprKind expr_kind)
            : Node(std::move(init_params.node_init_params)), expr_kind(std::move(init_params.expr_kind))
        {}

        ExprKind expr_kind;
    };

    enum class TypeKind {
        NAMED,
        TEMPLATE_INSTANTIATED,
        QUALIFIED,
    };

    struct Type : Node {
        Type(NodeInitParams init_params, TypeKind type_kind)
            : Node(std::move(init_params)), type_kind(type_kind)
        {}

        TypeKind type_kind;
    };

    //
    //
    //

    struct Visibility final : Node {
        Visibility(NodeInitParams init_params)
            : Node(std::move(init_params))
        {}

        // the target declaration and any symbols inside of it will have access.
        util::OptPtr<Decl> accessor;
    };

    struct Root final : Node {
        Root(NodeInitParams init_params)
            : Node(std::move(init_params))
        {}

        std::vector<util::FirmPtr<Decl>> decls;
    };

    // x
    struct Identifier final : Expr {
        Identifier(ExprInitParams init_params, IdentifierId identifier_id)
            : Expr(std::move(init_params), ExprKind::IDENTIFIER), identifier_id(identifier_id)
        {}
        
        IdentifierId identifier_id;
    };

    // "hello world"
    struct StringLiteral final : Expr {
        StringLiteral(ExprInitParams init_params, StringLiteralId string_literal_id)
            : Expr(std::move(init_params), ExprKind::STRING_LITERAL), string_literal_id(string_literal_id)
        {}

        StringLiteralId string_literal_id;
    };

    // struct NumberLiteral : Node {
    //     NumberLiteral(NodeInitParams init_params, manager::NumberLiteralId number_literal_id, tok::TokenType suffix)
    //         : Node(std::move(init_params)), number_literal_id(number_literal_id), suffix(suffix) {}

    //     manager::NumberLiteralId number_literal_id;
    //     tok::TokenType suffix;
    // };
    // ^^^^^^^^ update how number literals are stored during scan time before re-enabling this

    // -5
    struct UnaryExpr final : Expr {
        UnaryExpr(ExprInitParams init_params, util::FirmPtr<Expr> operand, tok::OperatorKind opr)
            : Expr(std::move(init_params), ExprKind::UNARY), operand(operand), opr(opr)
        {}

        util::FirmPtr<Expr> operand;
        tok::OperatorKind opr;
    };

    // 5 + 2
    struct BinaryExpr final : Expr {
        BinaryExpr(ExprInitParams init_params, util::FirmPtr<Expr> operand0, util::FirmPtr<Expr> operand1, tok::OperatorKind opr)
            : Expr(std::move(init_params), ExprKind::BINARY), operand0(operand0), operand1(operand1), opr(opr)
        {}

        util::FirmPtr<Expr> operand0;
        util::FirmPtr<Expr> operand1;
        tok::OperatorKind opr;
    };

    struct ScopeResolutionExpr;
    struct TemplateInstExpr;

    struct DeclReferenceExpr final : Expr {
        using ReferenceVariant = std::variant<util::FirmPtr<Identifier>, util::FirmPtr<ScopeResolutionExpr>>;

        DeclReferenceExpr(ExprInitParams init_params, ReferenceVariant reference)
            : Expr(std::move(init_params), ExprKind::DECL_REFERENCE), reference(std::move(reference))
        {}

        ReferenceVariant reference;
    };

    struct MemberAccessExpr;
    struct MemberReferenceExpr final : Expr {
        using ReferenceVariant = std::variant<util::FirmPtr<Identifier>, util::FirmPtr<MemberAccessExpr>>;

        MemberReferenceExpr(ExprInitParams init_params, ReferenceVariant reference)
            : Expr(std::move(init_params), ExprKind::MEMBER_REFERENCE), reference(std::move(reference))
        {}

        ReferenceVariant reference;
    };

    // math::pi
    struct ScopeResolutionExpr final : Expr {
        using Operand0Variant = std::variant<util::FirmPtr<DeclReferenceExpr>, util::FirmPtr<TemplateInstExpr>>;

        ScopeResolutionExpr(ExprInitParams init_params, Operand0Variant operand0, util::FirmPtr<Identifier> operand1)
            : Expr(std::move(init_params), ExprKind::SCOPE_RESOLUTION), operand0(operand0), operand1(operand1)
        {}

        Operand0Variant operand0;
        util::FirmPtr<Identifier> operand1;
    };
    
    // object.position.x
    struct MemberAccessExpr final : Expr {
        MemberAccessExpr(ExprInitParams init_params, util::FirmPtr<MemberReferenceExpr> operand0, util::FirmPtr<Identifier> operand1)
            : Expr(std::move(init_params), ExprKind::MEMBER_ACCESS), operand0(operand0), operand1(operand1)
        {}

        util::FirmPtr<MemberReferenceExpr> operand0;
        util::FirmPtr<Identifier> operand1;
    };

    // if a > b { } else { } <--- not actual compound statement, braces are parser resolved syntactic sugar
    struct TernaryExpr final : Expr {
        TernaryExpr(ExprInitParams init_params, util::FirmPtr<Expr> condition, util::FirmPtr<Expr> consequent, util::FirmPtr<Expr> alternate)
            : Expr(std::move(init_params), ExprKind::TERNARY), condition(condition), consequent(consequent), alternate(alternate)
        {}

        util::FirmPtr<Expr> condition;
        util::FirmPtr<Expr> consequent;
        util::FirmPtr<Expr> alternate;
    };

    struct CallExpr final : Expr {
        CallExpr(ExprInitParams init_params, util::FirmPtr<DeclReferenceExpr> callee, std::vector<util::FirmPtr<Expr>> args)
            : Expr(std::move(init_params), ExprKind::CALL), callee(callee), args(std::move(args))
        {}

        util::FirmPtr<DeclReferenceExpr> callee; // DeclReference
        std::vector<util::FirmPtr<Expr>> args; // {Expr}
    };

    struct HeapExpr final : Expr {
        HeapExpr(ExprInitParams init_params, util::FirmPtr<CallExpr> inst, util::OptPtr<Expr> address)
            : Expr(std::move(init_params), ExprKind::HEAP), inst(inst), address(std::move(address))
        {}

        util::FirmPtr<CallExpr> inst;
        util::OptPtr<Expr> address; // must semantically deduce to a pointer
    };

    struct TemplateArg;

    // use case: accessing a static member of a struct
    struct TemplateInstExpr final : Expr {
        TemplateInstExpr(ExprInitParams init_params, util::FirmPtr<DeclReferenceExpr> target, std::vector<util::FirmPtr<TemplateArg>> template_args)
            : Expr(std::move(init_params), ExprKind::TEMPLATE_inst), target(target), template_args(std::move(template_args))
        {}
        
        util::FirmPtr<DeclReferenceExpr> target;
        std::vector<util::FirmPtr<TemplateArg>> template_args;
    };

    struct NamedType final : Type {
        NamedType(NodeInitParams init_params, util::FirmPtr<DeclReferenceExpr> base)
            : Type(std::move(init_params), TypeKind::NAMED), base(base)
        {}

        util::FirmPtr<DeclReferenceExpr> base;
    };

    struct TemplateArg;
    struct TemplateInstantiatedType final : Type {
        TemplateInstantiatedType(NodeInitParams init_params, util::FirmPtr<NamedType> base, std::vector<util::FirmPtr<TemplateArg>> args)
            : Type(std::move(init_params), TypeKind::TEMPLATE_INSTANTIATED), base(base), args(std::move(args))
        {}

        util::FirmPtr<NamedType> base;
        std::vector<util::FirmPtr<TemplateArg>> args;
    };

    enum class TypeQualifier {
        MUT
    };

    struct QualifiedType final : Type {
        QualifiedType(NodeInitParams init_params, util::FirmPtr<Type> base, TypeQualifier qualifier)
            : Type(std::move(init_params), TypeKind::QUALIFIED), base(base), qualifier(qualifier)
        {}

        util::FirmPtr<Type> base;
        TypeQualifier qualifier;
    };

    struct GlobalDecl final : Decl {
        GlobalDecl(DeclInitParams init_params, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type, util::FirmPtr<Expr> value)
            : Decl(std::move(init_params), DeclKind::GLOBAL), storage_specifiers(storage_specifiers), name(name), type(type), value(value)
        {}

        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
        util::FirmPtr<Expr> value;
    };

    struct TypeTemplateParam final : Node {
        util::FirmPtr<Identifier> name;
    };

    struct ValueTemplateParam final : Node {
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };

    struct TemplateParam final : Node {
        using ValueVariant = std::variant<util::FirmPtr<TypeTemplateParam>, util::FirmPtr<ValueTemplateParam>>;

        TemplateParam(NodeInitParams init_params, ValueVariant value)
            : Node(std::move(init_params)), value(std::move(value))
        {}

        ValueVariant value;
    };

    struct TypeTemplateArg final : Node {
        TypeTemplateArg(NodeInitParams init_params, util::FirmPtr<Type> value)
            : Node(std::move(init_params)), value(value)
        {}

        util::FirmPtr<Type> value; // proven as compile-time constant by semantic analyzer
    };

    struct ValueTemplateArg final : Node {
        ValueTemplateArg(NodeInitParams init_params, util::FirmPtr<Expr> value)
            : Node(std::move(init_params)), value(value)
        {}
        
        util::FirmPtr<Expr> value;
    };

    struct TemplateArg final : Node {
        using ValueVariant = std::variant<util::FirmPtr<TypeTemplateArg>, util::FirmPtr<ValueTemplateArg>>;
        TemplateArg(NodeInitParams init_params, ValueVariant value)
            : Node(std::move(init_params)), value(std::move(value))
        {}

        ValueVariant value;
    };

    struct FunctionParam final : Node {
        FunctionParam(NodeInitParams init_params, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type)
            : Node(std::move(init_params)), name(name), type(type)
        {}

        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };
    
    struct Function final : Node {
        Function(NodeInitParams init_params, std::vector<util::FirmPtr<FunctionParam>> func_params, util::FirmPtr<Stmt> body, util::FirmPtr<Type> return_type)
            : Node(std::move(init_params)), func_params(std::move(func_params)), body(body), return_type(return_type)
        {}

        std::vector<util::FirmPtr<FunctionParam>> func_params;
        util::FirmPtr<Stmt> body;
        util::FirmPtr<Type> return_type;
    };

    struct FunctionTemplate final : Node {
        FunctionTemplate(NodeInitParams init_params, util::FirmPtr<Function> base, std::vector<util::FirmPtr<TemplateParam>> template_params)
            : Node(std::move(init_params)), base(base), template_params(std::move(template_params))
        {}

        util::FirmPtr<Function> base;
        std::vector<util::FirmPtr<TemplateParam>> template_params; // {TemplateParam}
    };
    
    struct FunctionDecl final : Decl {
        FunctionDecl(DeclInitParams init_params, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<FunctionTemplate> function_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(init_params), DeclKind::FUNCTION), storage_specifiers(storage_specifiers), function_template(function_template), name(name)
            {}

        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<FunctionTemplate> function_template;
        util::FirmPtr<Identifier> name;
    };

    struct Finalizer final : Node {
        Finalizer(NodeInitParams init_params, Function* function)
            : Node(std::move(init_params)), function(function)
        {}

        Function* function; // COMPLETELY CONCRETE
    };

    struct Method final : Node {
        Method(NodeInitParams init_params, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, FunctionTemplate* function_template)
            : Node(std::move(init_params)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), function_template(function_template)
        {}

        Visibility* visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        FunctionTemplate* function_template;
    };

    struct Property final : Node {
        Property(NodeInitParams init_params, util::FirmPtr<Visibility> visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type)
            : Node(std::move(init_params)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), type(type)
        {}

        util::FirmPtr<Visibility> visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };

    struct Struct final : Node {
        Struct(NodeInitParams init_params, std::vector<util::FirmPtr<Method>> methods, std::vector<util::FirmPtr<Property>> properties, std::vector<util::FirmPtr<Decl>> decls)
            : Node(std::move(init_params)), methods(std::move(methods)), properties(std::move(properties)), decls(std::move(decls))
        {}

        std::vector<util::FirmPtr<Method>> methods;
        std::vector<util::FirmPtr<Property>> properties;
        std::vector<util::FirmPtr<Decl>> decls;
    };

    struct StructTemplate final : Node {
        StructTemplate(NodeInitParams init_params, util::FirmPtr<Struct> base, std::vector<util::FirmPtr<TemplateParam>> template_params)
            : Node(std::move(init_params)), base(base), template_params(std::move(template_params))
        {}

        util::FirmPtr<Struct> base;
        std::vector<util::FirmPtr<TemplateParam>> template_params;
    };

    struct StructDecl final : Decl {
        StructDecl(DeclInitParams init_params, util::FirmPtr<StructTemplate> struct_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(init_params), DeclKind::STRUCT), struct_template(struct_template), name(name)
        {}

        util::FirmPtr<StructTemplate> struct_template;
        util::FirmPtr<Identifier> name;
    };

    struct TypeAliasTemplate final : Node {
        TypeAliasTemplate(NodeInitParams init_params, util::FirmPtr<Type> type, std::vector<util::FirmPtr<TemplateParam>> template_params)
            : Node(std::move(init_params)), type(type), template_params(std::move(template_params)) {}

        util::FirmPtr<Type> type;
        std::vector<util::FirmPtr<TemplateParam>> template_params;
    };

    struct TypeAliasDecl final : Decl {
        TypeAliasDecl(DeclInitParams init_params, util::FirmPtr<TypeAliasTemplate> type_alias_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(init_params), DeclKind::TYPE_ALIAS), type_alias_template(type_alias_template), name(name) {}

        util::FirmPtr<TypeAliasTemplate> type_alias_template;
        util::FirmPtr<Identifier> name;
    };

    struct ModuleDecl final : Decl {
        ModuleDecl(DeclInitParams init_params, util::FirmPtr<Identifier> name, std::vector<util::FirmPtr<Decl>> decls)
            : Decl(std::move(init_params), DeclKind::MODULE), name(name), decls(std::move(decls))
        {}
        
        util::FirmPtr<Identifier> name;
        std::vector<util::FirmPtr<Decl>> decls;

        // ^^^ in the symbol table, a module declaration's decls are split into decls and references.
    };

    // -- STATEMENTS

    struct ReturnStmt final : Stmt {
        ReturnStmt(NodeInitParams init_params, util::FirmPtr<Expr> value)
            : Stmt(std::move(init_params), StmtKind::RETURN), value(value)
        {}

        util::FirmPtr<Expr> value;
    };

    struct CompoundStmt final : Stmt {
        CompoundStmt(NodeInitParams init_params, std::vector<util::FirmPtr<Stmt>> stmts)
            : Stmt(std::move(init_params), StmtKind::COMPOUND), stmts(std::move(stmts))
        {}

        std::vector<util::FirmPtr<Stmt>> stmts;
    };
}