#pragma once

#include <vector>
#include <variant>

namespace liutil {
    template <typename ID, typename PBN, typename VAR> // PBN = polymorphic base node, VAR = a class with the property _raw that is a variant for all of the possible node types
    struct arena {
        arena() {}

        std::vector<VAR> list = {};

        template <typename T>
        inline ID insert(T&& node) {
            list.push_back(std::forward<T>(node));
            return list.size() - 1;
        }

        template <typename T>
        // Insert a node into the arena and get its type.
        inline T& static_insert(T&& node) {
            list.push_back(std::forward<T>(node));
            return std::get<T>(list.back()._raw);
        }

        inline void erase(const ID id) {
            list.erase(list.begin() + id);
        }
        
        template <typename T>
        inline T& get(const ID id) {
            return std::get<T>(list[id]._raw);
        }

        template <typename T>
        inline const T& get(const ID id) const {
            return std::get<T>(list[id]._raw);
        }


        // Not safe for long-term pointer usage. Only use for direct modification and disposal of the given pointer.
        inline PBN* get_base_ptr(const ID id) {
            return std::visit([](auto& n) { return (PBN*)(&n); }, list[id]._raw);
        }

        // Not safe for long-term pointer usage. Only use for direct access and disposal of the given pointer.
        inline const PBN* get_base_ptr(const ID id) const {
            return std::visit([](auto& n) { return (const PBN*)(&n); }, list[id]._raw);
        }

        template <typename T>
        inline T& get_as(ID id) {
            return std::get<T>(list[id]._raw);
        }

        template <typename T>
        inline const T& get_as(const ID id) const {
            return std::get<T>(list[id]._raw);
        }
    };
};