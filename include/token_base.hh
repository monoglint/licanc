/*

Holds top level declarations that are openly shared to any file that includes fcore.

*/

#pragma once

namespace fcore {
    enum class token_type {

    };

    struct t_token {
        t_token(t_spot&& spot)
            : spot(std::move(spot)) {}
        t_spot spot;
    };
}