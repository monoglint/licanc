#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>
#include <functional>

namespace util {
    template <class T_TAG, typename T_VALUE = std::size_t>
    requires std::is_integral_v<T_VALUE>
    struct SafeId {
        explicit SafeId(T_VALUE value)
            : value(value)
        {}

        SafeId()
            : value(INVALID_VALUE)
        {};

        // for getting the type when accessing a fully template-instantiated SafeId
        using ValueType = T_VALUE;
        using TagType = T_TAG;

        static constexpr T_VALUE INVALID_VALUE = std::numeric_limits<T_VALUE>::max();
        static const SafeId<T_TAG, T_VALUE> INVALID_ID;
        
        constexpr explicit operator T_VALUE() const {
            return value;
        }

        constexpr auto operator<=>(const SafeId<T_TAG, T_VALUE>&) const = default;

        [[nodiscard]]
        constexpr bool is_valid() const {
            return value != INVALID_VALUE;
        }

        [[nodiscard]]
        constexpr T_VALUE get() const {
            return value;
        }

    private:
        T_VALUE value = INVALID_VALUE;
    };

    template <class T_TAG, typename T_VALUE = std::size_t>
    struct IsSafeId : std::false_type {};

    template <class T_TAG, typename T_VALUE>
    struct IsSafeId<SafeId<T_TAG, T_VALUE>> : std::true_type {};

    template <class T_TAG, typename T_VALUE = std::size_t>
    constexpr inline bool IsSafeIdV = IsSafeId<T_TAG, T_VALUE>::value;

    template <class T_TAG, typename T_VALUE = std::size_t>
    concept c_is_safe_id = IsSafeIdV<T_TAG, T_VALUE>;
}

namespace std {
    template <class T_TAG, typename T_VALUE>
    struct hash<util::SafeId<T_TAG, T_VALUE>> {
        std::size_t operator()(const util::SafeId<T_TAG, T_VALUE>& id) const noexcept {
            return std::hash<T_VALUE>{}(id.get());
        }
    };
}