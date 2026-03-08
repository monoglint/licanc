#include "frontend/sema/sym.hh"

namespace frontend::sema {
    using ResolvedTypeDeclVariant = std::variant<sym::StructDecl*, sym::PrimativeDecl*>;
    struct ResolvedType {
        ResolvedTypeDeclVariant declaration;
        scan::ast::ResolvedTypeQualifier qualifier;

        constexpr auto operator<=>(const ResolvedType& other) const = default;
    };
}

namespace std {
    template<>
    struct hash<frontend::sema::ResolvedType> {
        std::size_t operator()(const frontend::sema::ResolvedType& resolved_type) const noexcept {
            std::size_t hash_val = std::hash<frontend::scan::ast::ResolvedTypeQualifier>{}(resolved_type.qualifier);

            util::combine_hashes(hash_val, std::hash<frontend::sema::ResolvedTypeDeclVariant>{}(resolved_type.declaration));
            
            return hash_val;
        }
    };
}