/*

a core component that the licanc library is based on. 
in previous projects, cstdint was used almost everywhere just for exact numeric types,
which is why this library was made

*/

#pragma once

// for typing
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
using f128 = long double;

namespace base {
    struct t_span {
        u32 start;
        u32 end;

        u32 line;
        u32 column;
    };
}
