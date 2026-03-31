/*

util:safe_id

This utility provides a type-safe hashable ID class that can be used for any context that requires the categorization of number-based ids.

@monoglint
30 March 2026

*/
module;

#include <cstddef>
#include <limits>
#include <type_traits>
#include <functional>

export module util:safe_id;

export namespace util {
    // Implicit construction results in an invalid value by default.
    template <class T_TAG, typename T_VALUE = std::size_t>
    requires std::is_integral_v<T_VALUE>
    class SafeId {
    public:
        explicit SafeId(T_VALUE value)
            : value(value)
        {}

        SafeId()
            : value(INVALID_VALUE)
        {};

        // for getting the type when accessing a fully template-instantiated SafeId
        using ValueType = T_VALUE;
        using TagType = T_TAG;

        // A constant that the safe id's value should be set to to represent invalidity.
        static constexpr T_VALUE INVALID_VALUE = std::numeric_limits<T_VALUE>::max();

        // Used as a QOL constant.
        static const SafeId INVALID_ID;
        
        constexpr explicit operator T_VALUE() const {
            return value;
        }

        constexpr auto operator<=>(const SafeId&) const = default;

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