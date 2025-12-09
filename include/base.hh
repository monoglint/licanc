/*

This header file holds fundemental data structures that the entire projet depends on.

*/

#pragma once

#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

namespace base {
    using t_success = bool;
    constexpr t_success SUCCESS = true;
    constexpr t_success FAILURE = false;
    
    template <typename T>
    struct arena {
        arena() {}
    };
}