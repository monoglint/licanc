#pragma once

#include "core.hh"
#include "token.hh"

namespace core {
    namespace parse {
        enum static_node_id : uint8_t {
            NODE_ARENA_ID
        };

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
    }
}