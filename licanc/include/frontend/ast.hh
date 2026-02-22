#pragma once

/*

INSTRUCTIONS FOR MODDING

Add a new t_ast_node_type value. Create a new struct that inherits from t_node.
Append the new struct type to the variant at the bottom of the file.

ENSURE THAT THE ORDER OF THE TYPE IN THE VARIANT LIST IS CONGRUENT TO THE ORDER OF THE ENUMS.
THAT IS HOW NODE TYPE IS IDENTIFIED!

*/

#include <variant>
#include <type_traits>

#include "base.hh"
#include "frontend/token.hh"

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

    // upon instantiation of this struct, set its identifier_id to the index of an interned string value
    struct t_expr_identifier : t_node {
        t_expr_identifier(base::t_span span, u64 identifier_id)
            : t_node(std::move(span)), identifier_id(identifier_id) {}
        
        // reference to within manager::t_compilation_unit
        u64 identifier_id; // t_identifier_id
    };

    struct t_expr_string_literal : t_node {
        t_expr_string_literal(base::t_span span, u64 string_literal_id)
            : t_node(std::move(span)), string_literal_id(string_literal_id) {}

        // reference to within manager::t_compilation_unit.
        u64 string_literal_id;
    };

    struct t_expr_unary : t_node {
        t_expr_unary(base::t_span span, t_node_id operand, token::t_token_type opr)
            : t_node(std::move(span)), operand(operand), opr(opr) {}

        t_node_id operand; // t_expr
        token::t_token_type opr;
    };

    struct t_expr_binary : t_node {
        t_expr_binary(base::t_span span, t_node_id operand0, t_node_id operand1, token::t_token_type opr)
            : t_node(std::move(span)), operand0(operand0), operand1(operand1), opr(opr) {}

        t_node_id operand0; // t_expr
        t_node_id operand1; // t_expr
        token::t_token_type opr;
    };

    struct t_expr_ternary : t_node {
        t_expr_ternary(base::t_span span, t_node_id condition, t_node_id consequent, t_node_id alternate, token::t_token_type opr)
            : t_node(std::move(span)), condition(condition), consequent(consequent), alternate(alternate), opr(opr) {}

        t_node_id condition; // t_expr
        t_node_id consequent; // t_expr
        t_node_id alternate; // t_expr
        token::t_token_type opr;
    };

    struct t_type : t_node {
        t_node_id source; // t_type | t_expr_identifier | t_expr_binary (scope resolution)
        t_node_ids arguments;

        token::t_token_type qualifier;
    };

    struct t_item_import : t_node {
        t_item_import(base::t_span span, t_node_id file_path)
            : t_node(std::move(span)), file_path(file_path) {}

        t_node_id file_path; // t_expr_string
    };

    struct t_item_global_declaration : t_node {
        t_item_global_declaration(base::t_span span, t_node_id name, t_node_id type, t_node_id value)
            : t_node(std::move(span)), name(name), type(type), value(value) {}

        t_node_id name; // t_expr_identifier
        t_node_id type; // t_type
        t_node_id value; // t_expr
    };

    struct t_template_parameter : t_node {
        t_node_id name; // t_expr_identifier
    };

    struct t_function : t_node {
        t_node_ids parameters; // ???
        t_node_id body; // t_stmt
        t_node_id return_type; // t_type

        t_node_ids template_arguments; // {t_type}
    };
    
    struct t_item_function_declaration : t_node {
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
    };

    enum class t_access_specifier {
        PRIVATE,
        PUBLIC
    };

    struct t_method : t_node {
        t_access_specifier access_specifier;
        t_node_id function_declaration; // t_item_function_declaration
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

    struct t_item_struct_declaration : t_node {
        t_node_id base; // t_struct
        t_node_ids specializations; // {t_struct}
        t_node_ids template_parameters; // {t_template_parameter}

        t_node_id name; // t_identifier
    };

    // make this support templates later. save your life bro
    struct t_item_type_declaration : t_node {
        t_item_type_declaration(base::t_span span, t_node_id name, t_node_id type)
            : t_node(std::move(span)), name(name), type(type) {}

        t_node_id name, type; // expr_identifier, t_type
    };

    // -- STATEMENTS

    //
    //
    //

    using t_node_variation = std::variant<
        t_none,
        t_expr_identifier,
        t_expr_string_literal,
        t_expr_unary,
        t_expr_binary,
        t_expr_ternary,
        t_type,
        t_item_import,
        t_item_global_declaration,
        t_template_parameter,
        t_function,
        t_item_function_declaration,
        t_constructor,
        t_method,
        t_property,
        t_struct,
        t_item_struct_declaration,
        t_item_type_declaration
    >;

    enum class t_ast_node_type {
        NONE,
        EXPR_IDENTIFIER,
        EXPR_STRING_LITERAL,
        EXPR_UNARY,
        EXPR_BINARY,
        EXPR_TERNARY,
        TYPE,
        ITEM_IMPORT,
        ITEM_GLOBAL_DECLARATION,
        TEMPLATE_PARAMETER,
        FUNCTION,
        ITEM_FUNCTION_DECLARATION,
        CONSTRUCTOR,
        METHOD,
        PROPERTY,
        STRUCT,
        ITEM_STRUCT_DECLARATION,
        ITEM_TYPE_DECLARATION,
    };

    // note: this is initialized before parsing or even lexing occurs. DESIGN IT TO WORK THAT WAY, FUTURE ME!!
    using t_ast = util::t_variant_base_deque<t_node_variation, t_node, t_node_id>;
}