#pragma once

#include <cstdint>

namespace core {
    namespace sema {
        enum static_symbol_id : uint8_t {
            SYM_INVALID_ID,
            SYM_ROOT_ID,
            SYM_GLOBAL_MODULE_ID,

            SYM_TI32_ID,
            SYM_TF32_ID,
        };

        // semantic context
        enum class semcon {
            FUNC,
            STRUCT,
        };
    }
}