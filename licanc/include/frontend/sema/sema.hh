#pragma once

#include "frontend/sema/sym.hh"

#include "util/vector_hasher.hh"

namespace frontend::sema {
    enum class t_type_name_qualifier {
        IMMUT,
        SOLID_REF,
        FLUID_REF,
    };

    using t_type_name_decl_variant = std::variant<sym::t_record_decl*, sym::t_primative_decl*>;
    struct t_type_name {
        t_type_name_decl_variant declaration;
        t_type_name_qualifier qualifier;
        std::vector<sym::t_template_argument*> template_arguments;

        constexpr inline auto operator<=>(const t_type_name& other) const = default;
    };
}

namespace std {
    template<>
    struct hash<frontend::sema::t_type_name> {
        std::size_t operator()(const frontend::sema::t_type_name& type_name) const noexcept {
            std::size_t hash_val = std::hash<frontend::sema::t_type_name_qualifier>{}(type_name.qualifier);

            util::combine_hashes(hash_val, std::hash<frontend::sema::t_type_name_decl_variant>{}(type_name.declaration));
            util::combine_hashes(hash_val, util::t_vector_hasher<frontend::sema::sym::t_template_argument*>{}(type_name.template_arguments));
            
            return hash_val;
        }
    };
}