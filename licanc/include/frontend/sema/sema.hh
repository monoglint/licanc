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
        sym::t_sym_reference declaration; // t_struct_declaration || t_primative_declaration
        t_type_name_qualifier qualifier;
        sym::t_sym_reference template_arguments; // {t_template_argument}
    };

    struct t_type_name_hasher {
        size_t operator()(const t_type_name& type_name) const noexcept {
            size_t hash_val = (size_t)type_name.qualifier;

            hash_val ^= std::hash<sym::t_sym_reference>{}(type_name.declaration) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            hash_val ^= util::vector_hasher<sym::t_sym_reference>{}(type_name.template_arguments) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            
            return hash_val;
        }
    };
}