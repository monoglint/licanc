#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>
#include <functional>

namespace util {
    template <class T_TAG, typename T_VALUE = std::size_t>
    requires std::is_integral_v<T_VALUE>
    struct t_safe_id {
        template <typename T>
        requires std::is_integral_v<T>
        t_safe_id(T value)
            : value(value) {}

        t_safe_id() = default;

        // for getting the type when accessing a fully template-instantiated t_safe_id
        using t_value_type = T_VALUE;

        static constexpr T_VALUE INVALID = std::numeric_limits<T_VALUE>::max();
        
        constexpr explicit inline operator T_VALUE() const {
            return value;
        }

        constexpr inline void operator=(const T_VALUE& new_value) {
            value = new_value;
        }

        constexpr inline auto operator<=>(const t_safe_id&) const = default;

        // constexpr inline bool operator==(const T_VALUE& other_value) const {
        //     return value == other_value;
        // }

        // constexpr inline bool operator!=(const T_VALUE& other_value) const {
        //     return value != other_value;
        // }

        // constexpr inline bool operator==(const t_safe_id<T_TAG, T_VALUE>& other) const {
        //     return value == other.value;
        // }

        // constexpr inline bool operator!=(const t_safe_id<T_TAG, T_VALUE>& other) const {
        //     return !(*this == other);
        // }

        // constexpr inline bool operator>(const t_safe_id<T_TAG, T_VALUE>& other) const {
        //     return value > other.value;
        // }

        // constexpr inline bool operator<(const t_safe_id<T_TAG, T_VALUE>& other) const {
        //     return value < other.value;
        // }

        // constexpr inline bool operator>=(const t_safe_id<T_TAG, T_VALUE>& other) const {
        //     return value >= other.value;
        // }

        // constexpr inline bool operator<=(const t_safe_id<T_TAG, T_VALUE>& other) const {
        //     return value <= other.value;
        // }

        constexpr inline bool is_valid() const {
            return value != INVALID;
        }

        constexpr inline T_VALUE get() const {
            return value;
        }

    private:
        T_VALUE value = INVALID;
    };

    template <class T_SAFE_ID>
    struct t_safe_id_hasher {
        std::size_t operator()(const T_SAFE_ID& id) const noexcept {
            return std::hash<typename T_SAFE_ID::t_value_type>{}(id.get());
        }
    };
}