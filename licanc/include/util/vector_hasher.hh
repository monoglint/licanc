#pragma once

#include <vector>
#include <functional>
#include "util/hash.hh"

/*

// Source - https://stackoverflow.com/a/27216842
// Posted by HolKann, modified by community. See post 'Timeline' for change history
// Retrieved 2026-02-23, License - CC BY-SA 4.0
// Adapted by @monoglint to support templates

*/
namespace util {
    // must stand unique. can not be an std::hash specializtion due to rules
    template <typename T, class T_HASHER = std::hash<T>>
    struct VectorHasher {
        using vector_of_t = std::vector<T>;

        inline std::size_t operator()(const vector_of_t& vec) const noexcept {
            std::size_t hash_val = vec.size();
            
            for (const T& i : vec) {
                util::combine_hashes(hash_val, T_HASHER{}(i));
            }

            return hash_val;
        }
    };
}

/*

note: hasher above apparently has high collision rates according to @see (stack overflow id: 5302813)
he proposes this solution:

// Source - https://stackoverflow.com/a/72073933
// Posted by see
// Retrieved 2026-02-23, License - CC BY-SA 4.0

std::std::size_t operator()(std::vector<uint32_t> const& vec) const {
  std::std::size_t seed = vec.size();
  for(auto x : vec) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

*/