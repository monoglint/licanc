/*

A container designed to hold a dynamic number of objects that have a variant type.
The programmer has control over the variant of objects, the use of an enum to identify by order (HAZARDOUS, but it's just for me so whatever),
and the type alias used for identifying the index of individual objects.

USE FOR LICANC:
AST and symbol table

Is this the best way to do things?
Probably not

Is it like an arena enough?
Yes

*/

#pragma once

#include <deque>

namespace util {
    /*
   
    Note for both structs:
    VARIANT_ENUM removed due to promoting volitility in code.
    
    */

    /*
    T_VARIANT - The std::variant that contains the possible types in the arena.
    T_ID - a typename to use that represents the index of the type. this should honestly just be an alias for size_t of some kind    
    */
    template <typename T_VARIANT, typename T_ID>
    struct t_arena {
        std::deque<T_VARIANT> raw;

        template <typename T>
        inline T& get(T_ID node_id) {
            return std::get<T>(raw[node_id]);
        }

        template <typename T>
        inline const T& get(T_ID node_id) const {
            return std::get<T>(raw[node_id]);
        }

        template <typename T>
        inline bool is(T_ID node_id) const {
            return std::holds_alternative<T>(raw[node_id]);
        }

        // inline VARIANT_ENUM get_type(ID node_id) {
        //     return static_cast<VARIANT_ENUM>(raw[node_id].index());    
        // }

        template <typename T, typename... T_ARGS>
        inline T_ID emplace(T_ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<T_ARGS>(args)...);
            return raw.size() - 1;
        }

        template <typename T>
        inline T_ID push(T node) {
            raw.push_back(std::move(node));
            return raw.size() - 1;
        }
    };

    /*
    T_VARIANT - The std::variant that contains the possible types in the arena.
    T_ID - a typename to use that represents the index of the type. this should honestly just be an alias for size_t of some kind    
    */
    template <typename T_VARIANT, typename T_BASE, typename T_ID>
    struct t_base_arena {
        std::deque<T_VARIANT> raw;

        inline T_BASE& get_base(T_ID node_id) {
            return std::visit([](auto& n) -> T_BASE& { return static_cast<T_BASE&>(n); }, raw[node_id]);
        }

        inline const T_BASE& get_base(T_ID node_id) const {
            return std::visit([](auto& n) -> T_BASE& { return static_cast<T_BASE&>(n); }, raw[node_id]);
        }

        template <typename T>
        inline T& get(T_ID node_id) {
            return std::get<T>(raw[node_id]);
        }

        template <typename T>
        inline const T& get(T_ID node_id) const {
            return std::get<T>(raw[node_id]);
        }

        template <typename T>
        inline bool is(T_ID node_id) const {
            return std::holds_alternative<T>(raw[node_id]);
        }

        // inline VARIANT_ENUM get_type(ID node_id) {
        //     return static_cast<VARIANT_ENUM>(raw[node_id].index());    
        // }

        template <typename T, typename... T_ARGS>
        inline T_ID emplace(T_ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<T_ARGS>(args)...);
            return raw.size() - 1;
        }

        template <typename T>
        inline T_ID push(T node) {
            raw.push_back(std::move(node));
            return raw.size() - 1;
        }
    };
};