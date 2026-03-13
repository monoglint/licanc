/*

an enum options utility that allows additional functionality to be applied to specific enums

specific example

enum class testbitflag {
    NONE = 0,
    ONE = 1 << 0,
    TWO = 1 << 1,
    THREE = 1 << 2,
};

template<>
struct EnumOptions<testbitflag> {
    static constexpr bool IsBitFlag = true;
};

@monoglint
11 march 2026

*/

#pragma once
#include <cstddef>
#include <type_traits>

namespace util {
    template <typename ENUM>
    requires std::is_enum_v<ENUM>
    class BitFlags {
    public:
        using Values = ENUM;
        using UnderlyingType = std::underlying_type_t<ENUM>;

        constexpr BitFlags(ENUM value)
            : value(value)
        {}

        ENUM value;

        constexpr BitFlags<ENUM> operator|(const BitFlags<ENUM>& other) const noexcept {
            return static_cast<ENUM>(static_cast<UnderlyingType>(value) | static_cast<UnderlyingType>(other.value));
        }

        constexpr BitFlags<ENUM> operator&(const BitFlags<ENUM>& other) const noexcept {
            return static_cast<ENUM>(static_cast<UnderlyingType>(value) & static_cast<UnderlyingType>(other.value));
        }
    };
}