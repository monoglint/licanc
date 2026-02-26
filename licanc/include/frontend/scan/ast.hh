#pragma once

/*

INSTRUCTIONS FOR MODDING

Add a new t_ast_node_type value. Create a new struct that inherits from t_node.
Append the new struct type to the variant at the bottom of the file.

ENSURE THAT THE ORDER OF THE TYPE IN THE VARIANT LIST IS CONGRUENT TO THE ORDER OF THE ENUMS.
THAT IS HOW NODE TYPE IS IDENTIFIED!


General rules:
- Assume syntax follows C-style paradigm (Rust allows many different traditionally statement based keywords to create expressions)
- Naming an AST node a statment or expression does not directly affect functionality. It is only for reader understanding
    * which in turn indirectly affects functionality.
    - More specifically, the functionality of an AST node is ingrained in how the parser generates it, and how the semantic
      analyzer reads and modifies it. The naming of the struct itself is purely semantic.

- A statement or expression should be anything that operates in runtime.
    - Statements exist to perform an action like declaring a variable.
    - Expressions exist to evaluate to a certain value. Expressions can sometimes exist on statement level like calling a function.
- An item should be anything that directly affects the compile-time environment.
- If there is a node like an identifier or function parameter where its use is more complicated,
  a specific semantic note isn't directly needed.
- Always comment what the property of a node should be if it holds an id that references another node.


Basic ast for non template testing
t_ast:
    t_function_declaration_item
        name: "add"
        function_template: t_function_template:
            function: t_function
                parameters:
                    t_parameter:
                        name: "a"
                        type: "u8"
                        default_value: t_none
                    t_parameter:
                        name: "b"
                        type: "u8"
                        default_value: t_none
                return_type: "u8"

Basic ast for TEMPLATE testing
t_ast:
    t_function_declaration_item
        name: "add"
        function_template: t_function_template:
            template_parameters:
                t_template_parameter:
                    name: "T"
                    is_const_value: false // T is a typename
            function: t_function
                parameters:
                    t_parameter:
                        name: "a"
                        type: "T"
                        default_value: t_none
                    t_parameter:
                        name: "b"
                        type: "T"
                        default_value: t_none
                return_type: "T"


*/

#include <variant>
#include <type_traits>

#include "util/arena.hh"

#include "frontend/scan/token.hh"

#include "util/span.hh"
#include "util/safe_id.hh"

#include "frontend/manager_types.hh"

namespace frontend::scan::ast {
    using t_node_id = util::t_safe_id<struct t_node_id_tag>;

    // this is not declared in ast_types because it requires <vector>
    using t_node_ids = std::vector<t_node_id>;

    struct t_node {
        util::t_span span;
    };

    struct t_root : t_node {
        t_root()
            : t_node(util::t_span()) {}

        virtual ~t_root() = default;

        t_node_id global_module; // t_module_declaration_item
        // ^^^^ though technically not syntactic, it will make ast reading much easier.
    };

    struct t_none : t_node {
        t_none(util::t_span span)
            : t_node(std::move(span)) {}
    };

    // x
    struct t_identifier : t_node {
        t_identifier(util::t_span span, manager::t_identifier_id identifier_id)
            : t_node(std::move(span)), identifier_id(identifier_id) {}
        
        // reference to within manager::t_compilation_unit
        manager::t_identifier_id identifier_id; // t_identifier_id
    };

    // "hello world"
    struct t_string_literal : t_node {
        t_string_literal(util::t_span span, manager::t_string_literal_id string_literal_id)
            : t_node(std::move(span)), string_literal_id(string_literal_id) {}

        // reference to within manager::t_compilation_unit.
        manager::t_string_literal_id string_literal_id;
    };

    struct t_number_literal : t_node {
        t_number_literal(util::t_span span, manager::t_number_literal_id number_literal_id, token::t_token_type suffix)
            : t_node(std::move(span)), number_literal_id(number_literal_id), suffix(suffix) {}

        manager::t_number_literal_id number_literal_id;
        token::t_token_type suffix;
    };

    // -5
    struct t_unary_expr : t_node {
        t_unary_expr(util::t_span span, t_node_id operand, token::t_token_type opr)
            : t_node(std::move(span)), operand(operand), opr(opr) {}

        t_node_id operand; // expr
        token::t_token_type opr;
    };

    // 5 + 2
    struct t_binary_expr : t_node {
        t_binary_expr(util::t_span span, t_node_id operand0, t_node_id operand1, token::t_token_type opr)
            : t_node(std::move(span)), operand0(operand0), operand1(operand1), opr(opr) {}

        t_node_id operand0; // expr
        t_node_id operand1; // expr
        token::t_token_type opr;
    };

    // math..pi
    struct t_scope_resolution_expr : t_node {
        t_scope_resolution_expr(util::t_span span, t_node_id operand0, t_node_id operand1, t_node_ids template_arguments = {})
            : t_node(std::move(span)), operand0(operand0), operand1(operand1), template_arguments(template_arguments) {}

        t_node_id operand0; // expr
        t_node_id operand1; // t_identifier

        t_node_ids template_arguments; // {t_template_argument}
    };

    // a > b ? x : y
    struct t_ternary_expr : t_node {
        t_ternary_expr(util::t_span span, t_node_id condition, t_node_id consequent, t_node_id alternate, token::t_token_type opr)
            : t_node(std::move(span)), condition(condition), consequent(consequent), alternate(alternate), opr(opr) {}

        t_node_id condition; // expr
        t_node_id consequent; // expr
        t_node_id alternate; // expr
        token::t_token_type opr;
    };

    // a::b || a()
    struct t_scope_reference : t_node {
        t_scope_reference(util::t_span span, t_node_id reference)
            : t_node(std::move(span)), reference(reference) {}

        t_node_id reference; // t_identifier || t_scope_resolution_expr
    };

    /*
    
    templated initializer in templated struct scenario
    util..reference_wrapper<int>..new<float>()

    */

    struct t_call_expr : t_node {
        t_call_expr(util::t_span span, t_node_id callee, t_node_ids arguments = {}, t_node_ids template_arguments = {})
            : t_node(std::move(span)), callee(callee), arguments(arguments), template_arguments(template_arguments) {}

        t_node_id callee; // t_scope_reference
        t_node_ids arguments; // {t_expr}
        t_node_ids template_arguments; // {t_template_argument}
    };

    // array<u8>
    struct t_type : t_node {
        enum class t_qualifier {
            IMMUT,
            SOLID_REF,
            FLUID_REF,
        };

        t_type(util::t_span span, t_node_id source, t_node_ids template_arguments, t_qualifier qualifier)
            : t_node(std::move(span)), source(source), template_arguments(template_arguments), qualifier(qualifier) {}

        t_node_id source; // t_type | t_scope_reference
        t_node_ids template_arguments; // {t_template_argument}

        t_qualifier qualifier;
    };

    // import "math"
    struct t_import_item : t_node {
        t_import_item(util::t_span span, t_node_id file_path)
            : t_node(std::move(span)), file_path(file_path) {}

        t_node_id file_path; // t_string_literal_expr
    };

    // 
    struct t_global_declaration_item : t_node {
        t_global_declaration_item(util::t_span span, t_node_id name, t_node_id type, t_node_id value)
            : t_node(std::move(span)), name(name), type(type), value(value) {}

        t_node_id name; // expr_identifier
        t_node_id type; // t_type
        t_node_id value; // expr
    };

    struct t_template_parameter : t_node {
        t_template_parameter(util::t_span span, t_node_id name, bool is_const_value, t_node_id const_value_type)
            : t_node(std::move(span)), name(name), is_const_value(is_const_value), const_value_type(const_value_type) {}

        t_node_id name; // t_identifier

        bool is_const_value;
        t_node_id const_value_type; // t_type? - only if parameter_type is CONST_VALUE
    };

    struct t_template_argument : t_node {
        t_template_argument(util::t_span span, t_node_id value)
            : t_node(std::move(span)), value(value) {}

        // note: if value is a t_expr, then the value of the template argument must be proven to be a compile time constant
        t_node_id value; // t_type || t_expr
    };

    struct t_function_parameter : t_node {
        t_function_parameter(util::t_span span, t_node_id name, t_node_id type)
            : t_node(std::move(span)), name(name), type(type) {}

        t_node_id name; // t_identifier
        t_node_id type; // t_type
    };
    
    struct t_function : t_node {
        t_function(util::t_span span, t_node_ids parameters, t_node_id body, t_node_id return_type)
            : t_node(std::move(span)), parameters(parameters), body(body), return_type(return_type) {}

        t_node_ids parameters; // t_function_parameter
        t_node_id body; // t_stmt
        t_node_id return_type; // t_type
    };

    struct t_function_template : t_node {
        t_function_template(util::t_span span, t_node_id base, t_node_ids template_parameters)
            : t_node(std::move(span)), base(base), template_parameters(template_parameters) {}

        t_node_id base; // t_function
        t_node_ids template_parameters; // {t_template_parameter}
        // SPECIALIZATIONS MAP STORED IN SYMBOL TABLE
    };
    
    struct t_function_declaration_item : t_node {
        t_function_declaration_item(util::t_span span, t_node_id function_template, t_node_id name)
            : t_node(std::move(span)), function_template(function_template), name(name) {}

        t_node_id function_template; // t_function_template
        t_node_id name; // t_identifier
    };

    struct t_initializer : t_node {
        t_initializer(util::t_span span, t_node_id name, t_node_id function)
            : t_node(std::move(span)), name(name), function(function) {}

        t_node_id name; // t_identifier
        t_node_id function; // t_function - NO DEPENDENT TYPES
    };

    struct t_finalizer : t_node {
        t_finalizer(util::t_span span, t_node_id function)
            : t_node(std::move(span)), function(function) {}

        t_node_id function; // t_function - NO DEPENDENT TYPES
    };

    struct t_method : t_node {
        t_method(util::t_span span, token::t_token_type access_specifier, t_node_id name, t_node_id function_template)
            : t_node(std::move(span)), access_specifier(access_specifier), name(name), function_template(function_template) {}

        token::t_token_type access_specifier;
        t_node_id name; // t_identifier
        t_node_id function_template; // t_function_template
    };

    struct t_property : t_node {
        t_property(util::t_span span, token::t_token_type access_specifier, t_node_id name, t_node_id type)
            : t_node(std::move(span)), access_specifier(access_specifier), name(name), type(type) {}

        token::t_token_type access_specifier;
        t_node_id name; // t_identifier
        t_node_id type; // t_type
    };

    struct t_struct : t_node {
        t_struct(util::t_span span, t_node_ids methods, t_node_ids properties, t_node_ids initializers, t_node_id finalizer)
            : t_node(std::move(span)), methods(methods), properties(properties), initializers(initializers), finalizer(finalizer) {}

        t_node_ids methods; // {t_method}
        t_node_ids properties; // {t_property}
        t_node_ids initializers; // {t_initializer}
        t_node_id finalizer; // t_finalizer?
    };

    struct t_struct_template : t_node {
        t_struct_template(util::t_span span, t_node_id base, t_node_ids template_parameters)
            : t_node(std::move(span)), base(base), template_parameters(template_parameters) {}

        t_node_id base; // t_struct
        t_node_ids template_parameters; // {t_template_parameter}
        // SPECIALIZATIONS MAP STORED IN SYMBOL TABLE
    };

    struct t_struct_declaration_item : t_node {
        t_struct_declaration_item(util::t_span span, t_node_id struct_template, t_node_id name)
            : t_node(std::move(span)), struct_template(struct_template), name(name) {}

        t_node_id struct_template; // t_struct_template
        t_node_id name; // t_identifier
    };

    struct t_type_alias_template : t_node {
        t_type_alias_template(util::t_span span, t_node_id type, t_node_ids specializations, t_node_ids template_parameters)
            : t_node(std::move(span)), type(type), specializations(specializations), template_parameters(template_parameters) {}

        t_node_id type; // t_type
        t_node_ids specializations; // {t_type}
        t_node_ids template_parameters; // {t_template_parameters}
    };

    // make this support templates later. save your life bro
    struct t_type_alias_declaration_item : t_node {
        t_type_alias_declaration_item(util::t_span span, t_node_id type_alias_template, t_node_id name)
            : t_node(std::move(span)), type_alias_template(type_alias_template), name(name) {}

        t_node_id type_alias_template; // t_type_alias_template
        t_node_id name; // t_identifier
    };

    struct t_module_declaration_item : t_node {
        t_module_declaration_item(util::t_span span, t_node_id name, t_node_ids items)
            : t_node(std::move(span)), name(name), items(items) {}
        
        t_node_id name; // t_identifier
        t_node_ids items; // {t_item}

        // ^^^ in the symbol table, a module declaration's items are split into declarations and references.
    };

    // -- STATEMENTS

    struct t_return_stmt : t_node {
        t_return_stmt(util::t_span span, t_node_id value)
            : t_node(std::move(span)), value(value) {}

        t_node_id value; // expr
    };

    struct t_body_stmt : t_node {
        t_body_stmt(util::t_span span, t_node_ids stmts)
            : t_node(std::move(span)), stmts(stmts) {}

        t_node_ids stmts; // {stmt}
    };

    //
    //
    //

    using t_node_variation = std::variant<
        t_root,
        t_none,
        t_identifier,
        t_string_literal,
        t_number_literal,
        t_unary_expr,
        t_binary_expr,
        t_scope_resolution_expr,
        t_ternary_expr,
        t_type,
        t_import_item,
        t_global_declaration_item,
        t_template_parameter,
        t_function_parameter,
        t_function,
        t_function_template,
        t_function_declaration_item,
        t_initializer,
        t_finalizer,
        t_method,
        t_property,
        t_struct,
        t_struct_template,
        t_struct_declaration_item,
        t_type_alias_template,
        t_type_alias_declaration_item,
        t_module_declaration_item,
        t_return_stmt,
        t_body_stmt
    >;

    // note: this is initialized before parsing or even lexing occurs. DESIGN IT TO WORK THAT WAY, FUTURE ME!!
    using t_ast = util::t_base_arena<t_node_variation, t_node_id, t_node>;
}