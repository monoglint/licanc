#pragma once

#include <optional>
#include <functional>
#include <unordered_map>
#include <deque>

#include "util/safe_id.hh"

namespace util {
    template <typename T, util::c_is_safe_id T_ID, typename T_HASH = std::hash<T>>
    struct t_intern_pool {
        using t_get_result = std::optional<std::reference_wrapper<T>>;
        using t_const_get_result = std::optional<std::reference_wrapper<const T>>;

        inline t_get_result get(T_ID index) {
            if (static_cast<std::size_t>(index) >= list.size())
                return std::nullopt;

            return std::ref(list[static_cast<std::size_t>(index)]);
        }

        inline t_const_get_result get(T_ID index) const {
            if (static_cast<std::size_t>(index) >= list.size())
                return std::nullopt;

            return std::cref(list[static_cast<std::size_t>(index)]);
        }

        inline T_ID intern(T value) {
            auto itr = reverse_list.find(value);

            if (itr == reverse_list.end()) {
                list.push_back(std::move(value));

                T_ID new_index = list.size() - 1;

                reverse_list[list.back()] = new_index;
                return new_index;
            }

            return itr->second;
        }
    private:
        std::unordered_map<T, T_ID, T_HASH> reverse_list;
        std::deque<T> list;
    };
}