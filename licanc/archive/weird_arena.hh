/*

A container designed to hold a dynamic number of objects that have a variant type.
The programmer has control over the variant of objects, the use of an enum to identify by order (HAZARDOUS, but it's just for me so whatever),
and the type alias used for identifying the index of individual objects.

USE FOR LICANC:
AST and symbol table

Is this the best way to do things?
Probably not

Is it like an arena enough?
Yes, this is enough like an arena

*/

#pragma once

#include <deque>
#include <optional>
#include <functional>
#include <variant>

#include "util/safe_id.hh"

namespace util {
    namespace variant {
        // Source - https://stackoverflow.com/a/52303671
        // Posted by Bargor, modified by community. See post 'Timeline' for change history
        // Retrieved 2026-02-26, License - CC BY-SA 4.0
        // Further modified by @monoglint to support the compiler
        template<class T_VARIANT, typename T, std::size_t INDEX = 0>
        consteval std::size_t get_variant_index() {
            static_assert(std::variant_size_v<T_VARIANT> > INDEX, "Type not found in variant");

            if constexpr (INDEX == std::variant_size_v<T_VARIANT>)
                return INDEX;

            else if constexpr (std::is_same_v<std::variant_alternative_t<INDEX, T_VARIANT>, T>)
                return INDEX;

            else
                return get_variant_index<T_VARIANT, T, INDEX + 1>();
        }
    }

    // design note: this template could have the id as the first parameter and just have the alternatives be variadic. i am not implementing this right now
    // because i am focusing too hard on "infrastructure" perfection and i need to control that
    // -session 10
    template <class T_VARIANT, util::c_is_safe_id T_ID>
    struct Arena {
        template <class T>
        using GetResult = std::optional<std::reference_wrapper<T>>;
        using GetVariantResult = std::optional<std::reference_wrapper<T_VARIANT>>;
        using GetConstVariantResult = std::optional<std::reference_wrapper<const T_VARIANT>>;

        [[nodiscard]]
        GetVariantResult get_variant(T_ID node_id) {
            if (!does_index_exist(node_id))
                return std::nullopt;
                
            return raw[node_id.get()];
        }

        [[nodiscard]]
        GetConstVariantResult get_variant(T_ID node_id) const {
            if (!does_index_exist(node_id))
                return std::nullopt;
                
            return raw[node_id.get()];
        }

        // Get node_id casted as T
        template <class T>
        [[nodiscard]]
        GetResult<T> get(T_ID node_id) {
            GetVariantResult get_variant_result = get_variant(node_id);

            if (!get_variant_result.has_value())
                return std::nullopt;

            if (auto ptr_value = std::get_if<T>(&get_variant_result.value().get()))
                return std::ref(*ptr_value);
            else
                return std::nullopt;
        }

        // Get node_id casted as T
        template <class T>
        [[nodiscard]]
        GetResult<const T> get(T_ID node_id) const {
            GetConstVariantResult get_variant_result = get_variant(node_id);

            if (!get_variant_result.has_value())
                return std::nullopt;

            if (auto ptr_value = std::get_if<T>(&get_variant_result.value().get()))
                return std::cref(*ptr_value);
            else
                return std::nullopt;
        }

        [[nodiscard]]
        std::size_t get_size() const {
            return raw.size();
        }
        
        [[nodiscard]]
        bool does_index_exist(T_ID node_id) const {
            return node_id.get() < get_size();
        }
        
        // Returns the actively filled variant index for the given id
        std::optional<std::size_t> index_of(T_ID node_id) const {
            GetConstVariantResult get_variant_result = get_variant(node_id);

            if (!get_variant_result.has_value())
                return std::nullopt;

            return get_variant_result.value().get().index();
        }

        // Returns the variant index for a given type
        template <class T>
        consteval static const std::size_t get_index() {
            return variant::get_variant_index<T_VARIANT, T>();
        }

        template <class T>
        [[nodiscard]]
        bool is(T_ID node_id) const {
            GetConstVariantResult get_variant_result = get_variant(node_id);

            if (!get_variant_result.has_value())
                return false;

            return std::holds_alternative<T>(get_variant_result.value().get());
        }

        template <class T, typename... T_ARGS>
        T_ID emplace(T_ARGS&&... args) {
            raw.emplace_back(std::in_place_type<T>,std::forward<T_ARGS>(args)...);
            return T_ID{raw.size() - 1};
        }

        template <class T, typename... T_ARGS>
        T& emplace_get(T_ARGS&&... args) {
            emplace<T>(std::forward<T_ARGS>(args)...);
            return std::get<T>(raw.back());
        }

        template <class T>
        T_ID push(T node) {
            raw.push_back(std::move(node));
            return T_ID{raw.size() - 1};
        }

        std::deque<T_VARIANT>::iterator begin() {
            return raw.begin();
        }

        std::deque<T_VARIANT>::iterator end() {
            return raw.end();
        }
    protected:
        std::deque<T_VARIANT> raw;
    };

    /*
        T_BASE = A base type that all the classes in the variant inherit from.
    */

    // note: a different name might be preferrable because base implies that this is the base class when it isnt
    template <class T_VARIANT, class T_ID, class T_BASE>
    struct BaseArena : Arena<T_VARIANT, T_ID> {
        using ArenaType = Arena<T_VARIANT, T_ID>;
        
        using GetBaseResult = std::optional<std::reference_wrapper<T_BASE>>;
        using GetConstBaseResult = std::optional<std::reference_wrapper<const T_BASE>>;

        [[nodiscard]]
        GetBaseResult get_base(T_ID node_id) {
            typename ArenaType::GetVariantResult get_variant_result = this->get_variant(node_id);

            if (!get_variant_result.has_value())
                return std::nullopt;

            return std::visit([](auto& n) -> T_BASE& { return static_cast<T_BASE&>(n); }, get_variant_result.value().get());
        }

        [[nodiscard]]
        GetConstBaseResult get_base(T_ID node_id) const {
            typename ArenaType::GetConstVariantResult get_variant_result = this->get_variant(node_id);

            if (!get_variant_result.has_value())
                return std::nullopt;

            return std::visit([](auto& n) -> T_BASE& { return static_cast<T_BASE&>(n); }, get_variant_result.value().get());
        }
    };
}