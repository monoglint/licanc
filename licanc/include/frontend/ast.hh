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

*/

#include <variant>
#include <type_traits>

#include "base.hh"
#include "frontend/token.hh"

#include "frontend/manager_types.hh"

#include "util/variant_deque.hh"

namespace frontend::ast {
    enum class t_type_qualifier {
        MUT,
        SOLID_REF,
        FLUID_REF,
    };

    using t_node_id = u64;
    using t_node_ids = std::vector<t_node_id>;

    struct t_node {
        t_node(base::t_span span)
            : span(std::move(span)) {}

        base::t_span span;
    };

    struct t_none : t_node {
        t_none(base::t_span span)
            : t_node(std::move(span)) {}
    };

    // x
    struct t_identifier : t_node {
        t_identifier(base::t_span span, manager::t_identifier_id identifier_id)
            : t_node(std::move(span)), identifier_id(identifier_id) {}
        
        // reference to within manager::t_compilation_unit
        manager::t_identifier_id identifier_id; // t_identifier_id
    };

    // "hello world"
    struct t_string_literal : t_node {
        t_string_literal(base::t_span span, manager::t_string_literal_id string_literal_id)
            : t_node(std::move(span)), string_literal_id(string_literal_id) {}

        // reference to within manager::t_compilation_unit.
        manager::t_string_literal_id string_literal_id;
    };

    // -5
    struct t_unary_expr : t_node {
        t_unary_expr(base::t_span span, t_node_id operand, token::t_token_type opr)
            : t_node(std::move(span)), operand(operand), opr(opr) {}

        t_node_id operand; // expr
        token::t_token_type opr;
    };

    // 5 + 2
    struct t_binary_expr : t_node {
        t_binary_expr(base::t_span span, t_node_id operand0, t_node_id operand1, token::t_token_type opr)
            : t_node(std::move(span)), operand0(operand0), operand1(operand1), opr(opr) {}

        t_node_id operand0; // expr
        t_node_id operand1; // expr
        token::t_token_type opr;
    };

    // math..pi
    struct t_scope_resolution_expr : t_node {
        t_scope_resolution_expr(base::t_span span, t_node_id operand0, t_node_id operand1)
            : t_node(std::move(span)), operand0(operand0), operand1(operand1) {}

        t_node_id operand0; // expr
        t_node_id operand1; // t_identifier
    };

    // a > b ? x : y
    struct t_ternary_expr : t_node {
        t_ternary_expr(base::t_span span, t_node_id condition, t_node_id consequent, t_node_id alternate, token::t_token_type opr)
            : t_node(std::move(span)), condition(condition), consequent(consequent), alternate(alternate), opr(opr) {}

        t_node_id condition; // expr
        t_node_id consequent; // expr
        t_node_id alternate; // expr
        token::t_token_type opr;
    };

    // array<u8>
    struct t_type : t_node {
        t_node_id source; // t_type | t_identifier | t_scope_resolution_expr
        t_node_ids arguments;

        t_type_qualifier qualifier;
    };

    // import "math"
    struct t_import_item : t_node {
        t_import_item(base::t_span span, t_node_id file_path)
            : t_node(std::move(span)), file_path(file_path) {}

        t_node_id file_path; // t_string_literal_expr
    };

    struct t_global_declaration_item : t_node {
        t_global_declaration_item(base::t_span span, t_node_id name, t_node_id type, t_node_id value)
            : t_node(std::move(span)), name(name), type(type), value(value) {}

        t_node_id name; // expr_identifier
        t_node_id type; // t_type
        t_node_id value; // expr
    };

    struct t_template_parameter : t_node {
        t_node_id name; // expr_identifier
    };

    struct t_function_parameter : t_node {
        t_node_id name; // t_identifier
        t_node_id type; // t_type
        t_node_id default_value; // expr?
    };
    
    struct t_function : t_node {
        t_node_ids parameters; // t_function_parameter
        t_node_id body; // t_stmt
        t_node_id return_type; // t_type

        t_node_ids template_arguments; // {t_type}
    };
    
    struct t_function_declaration_item : t_node {
        t_node_id base; // t_function
        t_node_ids specializations; // {t_function}
        t_node_ids template_parameters; // {t_template_parameter}

        t_node_id name; // t_identifier
    };

    struct t_constructor : t_node {
        enum class t_constructor_type {
            NORMAL,
            SOLIDIFY, // move constructor
            COPY,
            DESTRUCTOR
        };

        t_node_id function; // t_function - NO DEPENDENT TYPES
        t_constructor_type type;
    };

    enum class t_access_specifier {
        PRIVATE,
        PUBLIC
    };

    struct t_method : t_node {
        t_access_specifier access_specifier;
        t_node_id function_declaration; // t_function_delcaration_item
    };

    struct t_property : t_node {
        t_access_specifier access_specifier;
        t_node_id type; // t_type
    };

    struct t_struct : t_node {
        t_node_ids methods; // {t_method}
        t_node_ids properties; // {t_property}

        t_node_ids template_arguments; // {t_type}
    };

    struct t_struct_declaration_item : t_node {
        t_node_id base; // t_struct
        t_node_ids specializations; // {t_struct}
        t_node_ids template_parameters; // {t_template_parameter}

        t_node_id name; // t_identifier
    };

    // make this support templates later. save your life bro
    struct t_type_declaration_item : t_node {
        t_type_declaration_item(base::t_span span, t_node_id name, t_node_id type)
            : t_node(std::move(span)), name(name), type(type) {}

        t_node_id name; // t_identifier
        t_node_id type; // t_type
    };

    // -- STATEMENTS

    struct t_return_stmt : t_node {
        t_node_id value; // expr
    };

    //
    //
    //

    using t_node_variation = std::variant<
        t_none,
        t_identifier,
        t_string_literal,
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
        t_function_declaration_item,
        t_constructor,
        t_method,
        t_property,
        t_struct,
        t_struct_declaration_item,
        t_type_declaration_item,
        t_return_stmt
    >;

    // note: this is initialized before parsing or even lexing occurs. DESIGN IT TO WORK THAT WAY, FUTURE ME!!
    using t_ast = util::t_variant_base_deque<t_node_variation, t_node, t_node_id>;
}