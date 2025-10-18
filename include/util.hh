/*

====================================================

Minor functions are are used in different locations of the compiler.

====================================================

*/

#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <functional>
#include <vector>
#include <iostream>

#if defined(__GNUC__) || defined(__clang__)
    #define UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
    #define UNREACHABLE() __assume(0)
#else
    #include <cstdlib>
    #define UNREACHABLE() std::abort()
#endif

namespace liutil {
    inline bool is_whitespace(const char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    inline std::string indent_repeat(const uint8_t level) {
        std::string buffer;

        for (uint8_t i = 0; i < level; i++) {
            buffer += " ";
        }

        return buffer;
    }

    template <typename K, typename V>
    inline const K& find_map_key_by_value(const std::unordered_map<K, V>& map, const V& value) {
        for (const auto& pair : map) {
            if (pair.second == value)
                return pair.first;
        }

        throw std::runtime_error("Value not found in map.");
    }

    template <typename K, typename V>
    inline const bool map_has_value(const std::unordered_map<K, V>& map, const V& value) {
        for (const auto& pair : map) {
            if (pair.second == value)
                return true;
        }

        return false;
    }

    // https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
    template <typename T>
    struct vector_hasher {
        size_t operator()(const std::vector<T>& vec) const {
            size_t seed = vec.size();
            
            for (const T& i : vec) {
                seed ^= std::hash<T>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }

            return seed;
        }
    };
}