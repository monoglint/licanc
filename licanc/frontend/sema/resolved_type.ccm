module;

#include <variant>

export module frontend.sema:resolved_type;

import util;
import :syms_fwd;

export namespace frontend::sema {
    enum class ResolvedTypeQualifier {
        MUT
    };

    struct ResolvedType {
        using TypeDeclVariant = std::variant<StructDecl*, PrimitiveDecl*>;
        TypeDeclVariant declaration;
        ResolvedTypeQualifier qualifier;

        constexpr auto operator<=>(const ResolvedType& other) const = default;
    };

    using ResolvedTypeIndex = util::SafeId<struct ResolvedTypeIdTag>;
    using ResolvedTypePool = util::InternPool<ResolvedType, ResolvedTypeIndex>;
}

export namespace std {
    template<>
    struct hash<frontend::sema::ResolvedType> {
        std::size_t operator()(const frontend::sema::ResolvedType& resolved_type) const noexcept {
            std::size_t hash_val = std::hash<frontend::sema::ResolvedTypeQualifier>{}(resolved_type.qualifier);

            util::combine_hashes(hash_val, std::hash<frontend::sema::ResolvedType::TypeDeclVariant>{}(resolved_type.declaration));
            
            return hash_val;
        }
    };
}