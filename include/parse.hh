#pragma once

#include <unordered_set>

#include "core.hh"
#include "token.hh"

// Delimits arguments, tables, etc.
constexpr auto LIST_DELIMITER_TOKEN = core::token_type::COMMA;

constexpr auto L_EXPR_DELIMITER_TOKEN = core::token_type::LPAREN;
constexpr auto R_EXPR_DELIMITER_TOKEN = core::token_type::RPAREN;

// Used by function parameters and arguments.
constexpr auto L_FUNC_DELIMITER_TOKEN = core::token_type::LPAREN;
constexpr auto R_FUNC_DELIMITER_TOKEN = core::token_type::RPAREN;

// Used by type parameters and arguments.
constexpr auto L_TEMPLATE_DELIMITER_TOKEN = core::token_type::LSQUARE;
constexpr auto R_TEMPLATE_DELIMITER_TOKEN = core::token_type::RSQUARE;

// Like C-style braces.
constexpr auto L_BODY_DELIMITER_TOKEN = core::token_type::LBRACE;
constexpr auto R_BODY_DELIMITER_TOKEN = core::token_type::RBRACE;

constexpr auto TYPE_DENOTER_TOKEN = core::token_type::COLON;
constexpr auto TYPE_POINTER_TOKEN = core::token_type::AT;
constexpr auto TYPE_LVALUE_REFERENCE_TOKEN = core::token_type::AMPERSAND;
constexpr auto TYPE_RVALUE_REFERENCE_TOKEN = core::token_type::DOUBLE_AMPERSAND;

// dec x = 5
constexpr auto ASSIGNMENT_TOKEN = core::token_type::EQUAL;

// x ? 5 : 2
constexpr auto TERNARY_CONDITION_TOKEN = core::token_type::QUESTION;
constexpr auto TERNARY_ELSE_TOKEN = core::token_type::COLON;

constexpr auto INITIALIZER_LIST_START_TOKEN = core::token_type::RPTR;
constexpr auto L_INITIALIZER_SET_DELIMITER_TOKEN = core::token_type::LPAREN;
constexpr auto R_INITIALIZER_SET_DELIMITER_TOKEN = core::token_type::RPAREN;

/*

====================================================

Basic sets for the binary expression segment of the parser

====================================================

*/

using t_token_set = std::unordered_set<core::token_type>;

static const t_token_set binary_scope_resolution_set = {
    core::token_type::DOUBLE_DOT,
};

static const t_token_set binary_member_access_set = {
    core::token_type::DOT,
};

static const t_token_set unary_post_set = {
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS
};

static const t_token_set unary_pre_set = {
    core::token_type::MINUS, // negate
    core::token_type::BANG, // not
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS,
    core::token_type::AT, // address of
    core::token_type::ASTERISK, // derference
};

static const t_token_set binary_exponential_set = {
    core::token_type::CARET,
};

static const t_token_set binary_multiplicative_set = {
    core::token_type::ASTERISK,
    core::token_type::SLASH,
    core::token_type::PERCENT,
};

static const t_token_set binary_additive_set = {
    core::token_type::PLUS,
    core::token_type::MINUS,
};

static const t_token_set binary_numeric_comparison_set = {
    core::token_type::LARROW,
    core::token_type::LESS_EQUAL,
    core::token_type::RARROW,
    core::token_type::GREATER_EQUAL,
};

static const t_token_set binary_direct_comparison_set = {
    core::token_type::DOUBLE_EQUAL,
    core::token_type::BANG_EQUAL,
};

static const t_token_set binary_and_set = {
    core::token_type::DOUBLE_AMPERSAND,
};

static const t_token_set binary_or_set = {
    core::token_type::DOUBLE_PIPE,
};

static const t_token_set binary_assignment_set = {
    core::token_type::EQUAL,
    core::token_type::PLUS_EQUAL,
    core::token_type::MINUS_EQUAL,
    core::token_type::ASTERISK_EQUAL,
    core::token_type::SLASH_EQUAL,
    core::token_type::PERCENT_EQUAL,
    core::token_type::CARET_EQUAL,
};

static const bool is_overridable_operator(const core::token_type type) {
    switch (type) {
        case core::token_type::DOUBLE_PLUS:
        case core::token_type::DOUBLE_MINUS:
        case core::token_type::MINUS:
        case core::token_type::BANG:
        case core::token_type::AT:
        case core::token_type::ASTERISK:
        case core::token_type::CARET:
        case core::token_type::SLASH:
        case core::token_type::PERCENT:
        case core::token_type::PLUS:
        case core::token_type::LARROW:
        case core::token_type::LESS_EQUAL:
        case core::token_type::RARROW:
        case core::token_type::GREATER_EQUAL:
        case core::token_type::DOUBLE_EQUAL:
        case core::token_type::BANG_EQUAL:
            return true;
        default:
            return false;
    }
}