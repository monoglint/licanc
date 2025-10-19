#pragma once

#include <cstdint>

enum STATIC_SYMBOLS : uint8_t {
    INVALID_SYMBOL_ID,
    ROOT_SYMBOL_ID,
    GLOBAL_MODULE_SYMBOL_ID,

    TYPE_I32_SYMBOL_ID,
    TYPE_F32_SYMBOL_ID,
};

// semantic context
enum class semcon {
    FUNC,
    STRUCT,
};