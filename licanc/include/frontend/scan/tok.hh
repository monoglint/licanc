#pragma once

#include <vector>

#include "util/span.hh"

/*

struct color3<T> {
    #rgb(r: fixed u8, g: fixed u8, b: fixed u8)

    r: fixed T
    g: fixed T
    b: fixed T
}

*/

namespace frontend::scan::tok {
    enum class TokenKind {
        NONE,

        IMPORT,
        
        COPY, // copy - generates copy initializer
        MOVE, // move - generates move initializer

        OPERATOR,
    };

    enum class OperatorKind {
        PLUS,
        MINUS,
        ASTERISK,
        SLASH,
        PERCENT,

        DOUBLE_PLUS,
        DOUBLE_MINUS,
        
        DOUBLE_AMPERSAND,
        DOUBLE_PIPE,
        
        BANG,

        BANG_EQUALS,
        DOUBLE_EQUALS,
        LEFT_ARROW,
        RIGHT_ARROW,
        LEFT_ARROW_EQUALS,
        RIGHT_ARROW_EQUALS,
    };

    enum class StorageSpecifierFlags : uint8_t {
        NONE        = 0,
        STATIC      = 1 << 0,
    };

    struct Token {
        util::Span span;

    };

    // note: this doesn't break naming rules of specifying type in names because the naming style is conventional
    // like "abstract syntax TREE", "symbol TABLE"
    using Tokens = std::vector<Token>;
}