#pragma once

#include <optional>
#include <functional>
#include <unordered_map>
#include <deque>

namespace util {
    template <typename T>
    struct t_intern_pool {
        using t_get_result = std::optional<std::reference_wrapper<T>>;

        inline t_get_result get(size_t index) {
            if (index >= list.size())
                return std::nullopt;

            return list[index];
        }

        inline size_t intern(T value) {
            auto& itr = reverse_list.find(value);

            if (itr == reverse_list.end()) {
                list.push_back(std::move(value));

                size_t new_index = list.size() - 1;

                reverse_list[list.back()] = new_index;
                return new_index;
            }

            return itr->second;
        }
    private:
        std::unordered_map<T, size_t> reverse_list;
        std::deque<T> list;
    };
}