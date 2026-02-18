#pragma once

#include <vector>

namespace frontend::token {
    enum class t_token_type {
        NONE,
    };

    struct t_token {

    };

    // note: this doesn't break naming rules of specifying type in names because the naming style is conventional
    // like "abstract syntax TREE", "symbol TABLE"
    using t_tokens = std::vector<t_token>;
}