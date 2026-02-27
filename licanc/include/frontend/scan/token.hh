#pragma once

#include <vector>

/*

struct color3<T> {
    #rgb(r: fixed u8, g: fixed u8, b: fixed u8)

    r: fixed T
    g: fixed T
    b: fixed T
}

*/

namespace frontend::scan::token {
    enum class t_token_type {
        NONE,

        IMPORT,

        POUND, // # - used for defining initializers and finalizers
        
        COPY, // copy - generates copy initializer
        MOVE, // move - generates move initializer

    };

    struct t_token {

    };

    // note: this doesn't break naming rules of specifying type in names because the naming style is conventional
    // like "abstract syntax TREE", "symbol TABLE"
    using t_tokens = std::vector<t_token>;
}