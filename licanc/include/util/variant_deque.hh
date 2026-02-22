/*

A container designed to hold a dynamic number of objects that have a variant type.
The programmer has control over the variant of objects, the use of an enum to identify by order (HAZARDOUS, but it's just for me so whatever),
and the type alias used for identifying the index of individual objects.

USE FOR LICANC:
AST and symbol table

Is this the best way to do things?
Probably not

*/

#pragma once

#include <deque>

namespace util {
    template <typename VARIANT, typename VARIANT_ENUM, typename ID = size_t>
    struct t_variant_deque {
        std::deque<VARIANT> raw;

        template <typename T>
        inline T& get(ID node_id) {
            return std::get<T>(raw[node_id]);
        }

        template <typename T>
        inline const T& get(ID node_id) const {
            return std::get<T>(raw[node_id]);
        }

        inline VARIANT_ENUM get_type(ID node_id) {
            return static_cast<VARIANT_ENUM>(raw[node_id].index());    
        }

        template <typename T, typename... ARGS>
        inline ID emplace(ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<ARGS>(args)...);
            return raw.size() - 1;
        }

        template <typename T>
        inline ID push(T node) {
            raw.push_back(std::move(node));
            return raw.size() - 1;
        }
    };

    template <typename VARIANT, typename VARIANT_ENUM, typename BASE, typename ID = size_t>
    struct t_variant_base_deque {
        std::deque<VARIANT> raw;

        inline BASE& get_base(ID node_id) {
            return std::visit([](auto& n) -> BASE& { return static_cast<BASE&>(n); }, raw[node_id]);
        }

        inline const BASE& get_base(ID node_id) const {
            return std::visit([](auto& n) -> BASE& { return static_cast<BASE&>(n); }, raw[node_id]);
        }

        template <typename T>
        inline T& get(ID node_id) {
            return std::get<T>(raw[node_id]);
        }

        template <typename T>
        inline const T& get(ID node_id) const {
            return std::get<T>(raw[node_id]);
        }

        inline VARIANT_ENUM get_type(ID node_id) {
            return static_cast<VARIANT_ENUM>(raw[node_id].index());    
        }

        template <typename T, typename... ARGS>
        inline ID emplace(ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<ARGS>(args)...);
            return raw.size() - 1;
        }

        template <typename T>
        inline ID push(T node) {
            raw.push_back(std::move(node));
            return raw.size() - 1;
        }
    };
};