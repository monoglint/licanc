#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>
#include <functional>

namespace util {
    template <class T_TAG, typename T_VALUE = std::size_t>
    requires std::is_integral_v<T_VALUE>
    struct t_safe_id {
        explicit t_safe_id(T_VALUE value)
            : value(value) {}

        t_safe_id()
            : value(INVALID_VALUE) {};

        // for getting the type when accessing a fully template-instantiated t_safe_id
        using t_value_type = T_VALUE;

        inline static constexpr T_VALUE INVALID_VALUE = std::numeric_limits<T_VALUE>::max();
        
        constexpr explicit inline operator T_VALUE() const {
            return value;
        }

        constexpr inline auto operator<=>(const t_safe_id&) const = default;

        [[nodiscard]]
        constexpr inline bool is_valid() const {
            return value != INVALID_VALUE;
        }

        [[nodiscard]]
        constexpr inline T_VALUE get() const {
            return value;
        }

    private:
        T_VALUE value;
    };

    template <class T_SAFE_ID>
    struct t_safe_id_hasher {
        std::size_t operator()(const T_SAFE_ID& id) const noexcept {
            return std::hash<typename T_SAFE_ID::t_value_type>{}(id.get());
        }
    };

    template <class T_TAG, typename T_VALUE = std::size_t>
    struct t_is_safe_id : std::false_type {};

    template <class T_TAG, typename T_VALUE>
    struct t_is_safe_id<t_safe_id<T_TAG, T_VALUE>> : std::true_type {};

    template <class T_TAG, typename T_VALUE = std::size_t>
    constexpr inline bool t_is_safe_id_v = t_is_safe_id<T_TAG, T_VALUE>::value;

    template <class T_TAG, typename T_VALUE = std::size_t>
    concept c_is_safe_id = t_is_safe_id_v<T_TAG, T_VALUE>;
}