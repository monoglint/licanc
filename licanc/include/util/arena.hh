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
#include <optional>
#include <functional>

namespace util {
    /*
        T_VARIANT - The std::variant that contains the possible types in the arena.
        T_ID - a typename to use that represents the index of the type. this should honestly just be an alias for std::size_t of some kind    
    */
    template <class T_VARIANT, class T_ID>
    struct t_arena {
        template <typename T>
        using t_get_result = std::optional<std::reference_wrapper<T>>;

        // Get node_id casted as T
        template <typename T>
        inline t_get_result<T> get(T_ID node_id) {
            if (!does_index_exist(node_id))
                return std::nullopt;

            return std::get<T>(raw[node_id.get()]);
        }

        // Get node_id casted as T
        template <typename T>
        inline const t_get_result<T> get(T_ID node_id) const {
            if (!does_index_exist(node_id))
                return std::nullopt;

            return std::get<T>(raw[node_id.get()]);
        }

        inline std::size_t get_size() const {
            return raw.size();
        }
        
        inline bool does_index_exist(T_ID node_id) const {
            return node_id < get_size();
        }

        template <typename T>
        inline bool is(T_ID node_id) const {
            if (!does_index_exist(node_id))
                return false;

            return std::holds_alternative<T>(raw[node_id.get()]);
        }

        template <typename T, typename... T_ARGS>
        inline T_ID emplace(T_ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<T_ARGS>(args)...);
            return raw.size() - 1;
        }

        template <typename T, typename... T_ARGS>
        inline T& emplace_get(T_ARGS&&... args) {
            T_ID id = emplace<T>(std::forward<T_ARGS>(args)...);
            return get<T>(id).value().get();
        }

        // Get the id of the next node that will be added.
        inline T_ID next() const {
            return raw.size();
        }

        template <typename T>
        inline T_ID push(T node) {
            raw.push_back(std::move(node));
            return raw.size() - 1;
        }
    private:
        std::deque<T_VARIANT> raw;
    };

    /*
        T_BASE = A base type that all the classes in the variant inherit from.
    */

    template <class T_VARIANT, class T_ID, class T_BASE>
    struct t_base_arena : t_arena<T_VARIANT, T_ID> {
        using t_get_base_result = std::optional<std::reference_wrapper<T_BASE>>;

        std::deque<T_VARIANT> raw;

        inline t_get_base_result get_base(T_ID node_id) {
            if (!does_index_exist(node_id))
                return std::nullopt;

            return std::visit([](auto& n) -> T_BASE& { return static_cast<T_BASE&>(n); }, raw[node_id.get()]);
        }

        inline const t_get_base_result get_base(T_ID node_id) const {
            if (!does_index_exist(node_id))
                return std::nullopt;

            return std::visit([](auto& n) -> T_BASE& { return static_cast<T_BASE&>(n); }, raw[node_id.get()]);
        }
    };
}