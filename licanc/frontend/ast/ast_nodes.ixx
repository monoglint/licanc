module;

#include <variant>
#include <vector>

export module frontend.ast:ast_nodes;

import driver_base;

import frontend.tok;
import util;

export namespace frontend::ast {
    using NodeId = util::SafeId<class NodeIdTag>;

    struct NodeInitParams {
        NodeInitParams(NodeInitParams init_params, NodeId id)
            : span(std::move(init_params.span)), id(std::move(id))
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
        SCOPE_REFERENCE,
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

    struct Visibility : Node {
        Visibility(NodeInitParams init_params)
            : Node(std::move(init_params))
        {}

        // the target declaration and any symbols inside of it will have access.
        util::OptPtr<Decl> accessor;
    };

    struct Root : Node {
        Root(NodeInitParams init_params)
            : Node(std::move(init_params))
        {}

        std::vector<util::FirmPtr<Decl>> decls;
    };

    // x
    struct Identifier : Expr {
        Identifier(ExprInitParams init_params, driver_base::IdentifierIndex identifier_index)
            : Expr(std::move(init_params), ExprKind::IDENTIFIER), identifier_index(identifier_index)
        {}
        
        driver_base::IdentifierIndex identifier_index;
    };

    // "hello world"
    struct StringLiteral : Expr {
        StringLiteral(ExprInitParams init_params, driver_base::StringLiteralIndex string_literal_index)
            : Expr(std::move(init_params), ExprKind::STRING_LITERAL), string_literal_index(string_literal_index)
        {}

        driver_base::StringLiteralIndex string_literal_index;
    };

    // struct NumberLiteral : Node {
    //     NumberLiteral(NodeInitParams init_params, manager::NumberLiteralId number_literal_id, tok::TokenType suffix)
    //         : Node(std::move(init_params)), number_literal_id(number_literal_id), suffix(suffix) {}

    //     manager::NumberLiteralId number_literal_id;
    //     tok::TokenType suffix;
    // };
    // ^^^^^^^^ update how number literals are stored during scan time before re-enabling this

    // -5
    struct UnaryExpr : Expr {
        UnaryExpr(ExprInitParams init_params, util::FirmPtr<Expr> operand, tok::OperatorKind opr)
            : Expr(std::move(init_params), ExprKind::UNARY), operand(operand), opr(opr)
        {}

        util::FirmPtr<Expr> operand;
        tok::OperatorKind opr;
    };

    // 5 + 2
    struct BinaryExpr : Expr {
        BinaryExpr(ExprInitParams init_params, util::FirmPtr<Expr> operand0, util::FirmPtr<Expr> operand1, tok::OperatorKind opr)
            : Expr(std::move(init_params), ExprKind::BINARY), operand0(operand0), operand1(operand1), opr(opr)
        {}

        util::FirmPtr<Expr> operand0;
        util::FirmPtr<Expr> operand1;
        tok::OperatorKind opr;
    };

    struct ScopeResolutionExpr;
    // a::b() || a()
    struct ScopeReferenceExpr : Expr {
        using ReferenceVariant = std::variant<util::FirmPtr<Identifier>, util::FirmPtr<ScopeResolutionExpr>>;

        ScopeReferenceExpr(ExprInitParams init_params, ReferenceVariant reference)
            : Expr(std::move(init_params), ExprKind::SCOPE_REFERENCE), reference(std::move(reference))
        {}

        ReferenceVariant reference;
    };

    struct MemberAccessExpr;
    struct MemberReferenceExpr : Expr {
        using ReferenceVariant = std::variant<util::FirmPtr<Identifier>, util::FirmPtr<MemberAccessExpr>>;

        MemberReferenceExpr(ExprInitParams init_params, ReferenceVariant reference)
            : Expr(std::move(init_params), ExprKind::MEMBER_REFERENCE), reference(std::move(reference))
        {}

        ReferenceVariant reference;
    };

    // math::pi
    struct ScopeResolutionExpr : Expr {
        ScopeResolutionExpr(ExprInitParams init_params, util::FirmPtr<ScopeReferenceExpr> operand0, util::FirmPtr<Identifier> operand1)
            : Expr(std::move(init_params), ExprKind::SCOPE_RESOLUTION), operand0(operand0), operand1(operand1)
        {}

        util::FirmPtr<ScopeReferenceExpr> operand0;
        util::FirmPtr<Identifier> operand1;
    };
    
    // object.position.x
    struct MemberAccessExpr : Expr {
        MemberAccessExpr(ExprInitParams init_params, util::FirmPtr<MemberReferenceExpr> operand0, util::FirmPtr<Identifier> operand1)
            : Expr(std::move(init_params), ExprKind::MEMBER_ACCESS), operand0(operand0), operand1(operand1)
        {}

        util::FirmPtr<MemberReferenceExpr> operand0;
        util::FirmPtr<Identifier> operand1;
    };

    // if a > b { } else { } <--- not actual compound statement, braces are parser resolved syntactic sugar
    struct TernaryExpr : Expr {
        TernaryExpr(ExprInitParams init_params, util::FirmPtr<Expr> condition, util::FirmPtr<Expr> consequent, util::FirmPtr<Expr> alternate)
            : Expr(std::move(init_params), ExprKind::TERNARY), condition(condition), consequent(consequent), alternate(alternate)
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
        CallExpr(ExprInitParams init_params, util::FirmPtr<ScopeReferenceExpr> callee, std::vector<util::FirmPtr<Expr>> arguments)
            : Expr(std::move(init_params), ExprKind::CALL), callee(callee), arguments(std::move(arguments))
        {}

        util::FirmPtr<ScopeReferenceExpr> callee; // ScopeReference
        std::vector<util::FirmPtr<Expr>> arguments; // {Expr}
    };

    struct HeapExpr : Expr {
        HeapExpr(ExprInitParams init_params, util::FirmPtr<CallExpr> inst, util::OptPtr<Expr> address)
            : Expr(std::move(init_params), ExprKind::HEAP), inst(inst), address(std::move(address))
        {}

        util::FirmPtr<CallExpr> inst;
        util::OptPtr<Expr> address; // must semantically deduce to a pointer
    };

    struct TemplateArgument;

    // use case: accessing a static member of a struct
    struct TemplateInstExpr : Expr {
        TemplateInstExpr(ExprInitParams init_params, util::FirmPtr<ScopeReferenceExpr> target, std::vector<TemplateArgument*> template_arguments)
            : Expr(std::move(init_params), ExprKind::TEMPLATE_inst), target(target), template_arguments(std::move(template_arguments))
        {}
        
        util::FirmPtr<ScopeReferenceExpr> target;
        std::vector<TemplateArgument*> template_arguments;
    };

    struct NamedType : Type {
        NamedType(NodeInitParams init_params, util::FirmPtr<ScopeReferenceExpr> base)
            : Type(std::move(init_params), TypeKind::NAMED), base(base)
        {}

        util::FirmPtr<ScopeReferenceExpr> base;
    };

    struct TemplateArgument;
    struct TemplateInstantiatedType : Type {
        TemplateInstantiatedType(NodeInitParams init_params, util::FirmPtr<NamedType> base, std::vector<TemplateArgument*> arguments)
            : Type(std::move(init_params), TypeKind::TEMPLATE_INSTANTIATED), base(base), arguments(std::move(arguments))
        {}

        util::FirmPtr<NamedType> base;
        std::vector<TemplateArgument*> arguments;
    };

    enum class TypeQualifier {
        MUT
    };

    struct QualifiedType : Type {
        QualifiedType(NodeInitParams init_params, util::FirmPtr<Type> base, TypeQualifier qualifier)
            : Type(std::move(init_params), TypeKind::QUALIFIED), base(base), qualifier(qualifier)
        {}

        util::FirmPtr<Type> base;
        TypeQualifier qualifier;
    };

    struct GlobalDecl : Decl {
        GlobalDecl(DeclInitParams init_params, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type, util::FirmPtr<Expr> value)
            : Decl(std::move(init_params), DeclKind::GLOBAL), storage_specifiers(storage_specifiers), name(name), type(type), value(value)
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

        TemplateParameter(NodeInitParams init_params, ValueVariant value)
            : Node(std::move(init_params)), value(std::move(value))
        {}

        ValueVariant value;
    };

    struct TypeTemplateArgument : Node {
        TypeTemplateArgument(NodeInitParams init_params, util::FirmPtr<Type> value)
            : Node(std::move(init_params)), value(value)
        {}

        util::FirmPtr<Type> value; // proven as compile-time constant by semantic analyzer
    };

    struct ValueTemplateArgument : Node {
        ValueTemplateArgument(NodeInitParams init_params, util::FirmPtr<Expr> value)
            : Node(std::move(init_params)), value(value)
        {}
        
        util::FirmPtr<Expr> value;
    };

    struct TemplateArgument : Node {
        using ValueVariant = std::variant<TypeTemplateArgument*, ValueTemplateArgument*>;
        TemplateArgument(NodeInitParams init_params, ValueVariant value)
            : Node(std::move(init_params)), value(std::move(value))
        {}

        ValueVariant value;
    };

    struct FunctionParameter : Node {
        FunctionParameter(NodeInitParams init_params, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type)
            : Node(std::move(init_params)), name(name), type(type)
        {}

        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };
    
    struct Function : Node {
        Function(NodeInitParams init_params, std::vector<FunctionParameter*> parameters, util::FirmPtr<Stmt> body, util::FirmPtr<Type> return_type)
            : Node(std::move(init_params)), parameters(std::move(parameters)), body(body), return_type(return_type)
        {}

        std::vector<FunctionParameter*> parameters;
        util::FirmPtr<Stmt> body;
        util::FirmPtr<Type> return_type;
    };

    struct FunctionTemplate : Node {
        FunctionTemplate(NodeInitParams init_params, util::FirmPtr<Function> base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(init_params)), base(base), template_parameters(std::move(template_parameters))
        {}

        util::FirmPtr<Function> base;
        std::vector<TemplateParameter*> template_parameters; // {TemplateParameter}
    };
    
    struct FunctionDecl : Decl {
        FunctionDecl(DeclInitParams init_params, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<FunctionTemplate> function_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(init_params), DeclKind::FUNCTION), storage_specifiers(storage_specifiers), function_template(function_template), name(name)
            {}

        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<FunctionTemplate> function_template;
        util::FirmPtr<Identifier> name;
    };

    struct Finalizer : Node {
        Finalizer(NodeInitParams init_params, Function* function)
            : Node(std::move(init_params)), function(function)
        {}

        Function* function; // COMPLETELY CONCRETE
    };

    struct Method : Node {
        Method(NodeInitParams init_params, Visibility* visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, FunctionTemplate* function_template)
            : Node(std::move(init_params)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), function_template(function_template)
        {}

        Visibility* visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        FunctionTemplate* function_template;
    };

    struct Property : Node {
        Property(NodeInitParams init_params, util::FirmPtr<Visibility> visibility, tok::StorageSpecifierFlags storage_specifiers, util::FirmPtr<Identifier> name, util::FirmPtr<Type> type)
            : Node(std::move(init_params)), visibility(visibility), storage_specifiers(storage_specifiers), name(name), type(type)
        {}

        util::FirmPtr<Visibility> visibility;
        tok::StorageSpecifierFlags storage_specifiers;
        util::FirmPtr<Identifier> name;
        util::FirmPtr<Type> type;
    };

    struct Struct : Node {
        Struct(NodeInitParams init_params, std::vector<Method*> methods, std::vector<Property*> properties, Finalizer* finalizer)
            : Node(std::move(init_params)), methods(std::move(methods)), properties(std::move(properties)), finalizer(finalizer)
        {}

        std::vector<Method*> methods;
        std::vector<Property*> properties;
        // TODO: additional optional declarations need to be added like static variables, imports, etc.
        
        Finalizer* finalizer; // WHEN TRAITS ADDED, DONT HAVE DESTRUCTORS HERE!!
    };

    struct StructTemplate : Node {
        StructTemplate(NodeInitParams init_params, Struct* base, std::vector<TemplateParameter*> template_parameters)
            : Node(std::move(init_params)), base(base), template_parameters(std::move(template_parameters))
        {}

        Struct* base;
        std::vector<TemplateParameter*> template_parameters;
    };

    struct StructDecl : Decl {
        StructDecl(DeclInitParams init_params, util::FirmPtr<StructTemplate> struct_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(init_params), DeclKind::STRUCT), struct_template(struct_template), name(name)
        {}

        util::FirmPtr<StructTemplate> struct_template;
        util::FirmPtr<Identifier> name;
    };

    struct TypeAliasTemplate : Node {
        TypeAliasTemplate(NodeInitParams init_params, util::FirmPtr<Type> type, std::vector<util::FirmPtr<TemplateParameter>> template_parameters)
            : Node(std::move(init_params)), type(type), template_parameters(std::move(template_parameters)) {}

        util::FirmPtr<Type> type;
        std::vector<util::FirmPtr<TemplateParameter>> template_parameters;
    };

    struct TypeAliasDecl : Decl {
        TypeAliasDecl(DeclInitParams init_params, util::FirmPtr<TypeAliasTemplate> type_alias_template, util::FirmPtr<Identifier> name)
            : Decl(std::move(init_params), DeclKind::TYPE_ALIAS), type_alias_template(type_alias_template), name(name) {}

        util::FirmPtr<TypeAliasTemplate> type_alias_template;
        util::FirmPtr<Identifier> name;
    };

    struct ModuleDecl : Decl {
        ModuleDecl(DeclInitParams init_params, util::FirmPtr<Identifier> name, std::vector<util::FirmPtr<Decl>> decls)
            : Decl(std::move(init_params), DeclKind::MODULE), name(name), decls(std::move(decls))
        {}
        
        util::FirmPtr<Identifier> name;
        std::vector<util::FirmPtr<Decl>> decls;

        // ^^^ in the symbol table, a module declaration's decls are split into decls and references.
    };

    // -- STATEMENTS

    struct ReturnStmt : Stmt {
        ReturnStmt(NodeInitParams init_params, util::FirmPtr<Expr> value)
            : Stmt(std::move(init_params), StmtKind::RETURN), value(value)
        {}

        util::FirmPtr<Expr> value;
    };

    struct CompoundStmt : Stmt {
        CompoundStmt(NodeInitParams init_params, std::vector<Stmt*> stmts)
            : Stmt(std::move(init_params), StmtKind::COMPOUND), stmts(std::move(stmts))
        {}

        std::vector<Stmt*> stmts;
    };
}