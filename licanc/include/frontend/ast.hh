#pragma once

/*

INSTRUCTIONS FOR MODDING

Add a new t_ast_node_type value. Create a new struct that inherits from t_node.
Append the new struct type to the variant at the bottom of the file.

ENSURE THAT THE ORDER OF THE TYPE IN THE VARIANT LIST IS CONGRUENT TO THE ORDER OF THE ENUMS.
THAT IS HOW NODE TYPE IS IDENTIFIED!

*/

#include <deque>
#include <variant>
#include <type_traits>

#include "base.hh"
#include "frontend/token.hh"

namespace frontend::ast {
    enum class t_ast_node_type {
        // -- EXPRESSIONS

        // embeddable

        EXPR_IDENTIFIER,
        EXPR_STRING_LITERAL,
        EXPR_UNARY,
        EXPR_BINARY,
        EXPR_TERNARY,

        // -- EXPRESSION DEPENDENT MISC

        TYPE,

        // -- ITEMS

        ITEM_IMPORT,
        ITEM_DECLARATION,
        ITEM_TYPE_DECLARATION,
        
        // -- STATEMENTS

        // control flow

        STMT_IF,
        STMT_WHILE,
        STMT_FOR,
        STMT_EXPRESSION,

        // instance

        STMT_DECLARATION,
        STMT_CHUNK,


    };

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

    // -- EXPRESSIONS

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

    // -- ITEMS

    struct t_item_import : t_node {
        t_item_import(base::t_span span, t_node_id file_path)
            : t_node(std::move(span)), file_path(file_path) {}

        t_node_id file_path; // t_expr_string
    };

    struct t_item_declaration : t_node {
        t_item_declaration(base::t_span span, t_node_id name, t_node_id type, t_node_id value)
            : t_node(std::move(span)), name(name), type(type), value(value) {}

        t_node_id name; // t_expr_identifier
        t_node_id type; // t_type
        t_node_id value; // t_expr
    };

    struct t_item_type_declaration : t_node {
        t_item_type_declaration(base::t_span span, t_node_id name, t_node_id type)
            : t_node(std::move(span)), name(name), type(type) {}

        t_node_id name, type; // expr_identifier, t_type
    };

    // -- STATEMENTS

    struct t_stmt_if : t_node {

    };

    struct t_stmt_while : t_node {

    };

    struct t_stmt_for : t_node {

    };

    struct t_stmt_expression : t_node {

    };

    struct t_stmt_declaration : t_node {

    };

    struct t_stmt_chunk : t_node {

    };

    //
    //
    //

    using t_node_variation = std::variant<
        t_expr_identifier,
        t_expr_string_literal,
        t_expr_unary,
        t_expr_binary,
        t_expr_ternary,

        t_type,

        t_item_import,
        t_item_declaration,
        t_item_type_declaration,

        t_stmt_if,
        t_stmt_while,
        t_stmt_for,
        t_stmt_expression,
        t_stmt_declaration,
        t_stmt_chunk
    >;

    // note: this is initialized before parsing or even lexing occurs. DESIGN IT TO WORK THAT WAY, FUTURE ME!!
    struct t_ast {
        std::deque<t_node_variation> raw;

        inline t_node& get_base(t_node_id node_id) {
            return std::visit([](auto& n) -> t_node& { return static_cast<t_node&>(n); }, raw[node_id]);
        }

        template <typename T>
        inline T& get(t_node_id node_id) {
            return std::get<T>(raw[node_id]);
        }

        inline t_ast_node_type get_type(t_node_id node_id) {
            return (t_ast_node_type)raw[node_id].index();    
        }

        template <typename T, typename... ARGS>
        inline t_node_id emplace(ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<ARGS>(args)...);
            return raw.size() - 1;
        }

        template <typename T>
        inline t_node_id push(T node) {
            raw.push_back(std::move(node));
            return raw.size() - 1;
        }
    };
}