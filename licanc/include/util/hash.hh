#pragma once

namespace util {
    inline void hash_combine(u64& seed, u64 new_hash) {
        seed ^= new_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}