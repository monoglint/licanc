#pragma once

#include "frontend/sema/sym.hh"
#include "util/vector_hasher.hh"

namespace frontend::sema {
    enum class t_type_name_qualifier {
        IMMUT,
        SOLID_REF,
        FLUID_REF,
    };

    // interned in t_compilation_unit. this is NOT A SYMBOL.
    struct t_type_name {
        sym::t_reference declaration; // t_struct_declaration || t_primative_declaration
        t_type_name_qualifier qualifier;
        sym::t_reference template_arguments; // {t_template_argument}
    };

    struct t_type_name_hasher {
        std::size_t operator()(const t_type_name& type_name) const noexcept {
            std::size_t hash_val = std::hash<t_type_name_qualifier>{}(type_name.qualifier);

            util::combine_hashes(hash_val, std::hash<sym::t_reference>{}(type_name.declaration));
            util::combine_hashes(hash_val, std::hash<sym::t_reference>{}(type_name.template_arguments));
            
            return hash_val;
        }
    };
}