#include "util/vector_hasher.hh"

#include "util/hash.hh"

template <typename T, class T_HASHER>
std::size_t util::t_vector_hasher<T, T_HASHER>::operator()(const vector_of_t& vec) const noexcept {
    std::size_t hash_val = vec.size();
    
    for (const T& i : vec) {
        util::combine_hashes(hash_val, T_HASHER{}(i));
    }

    return hash_val;
}