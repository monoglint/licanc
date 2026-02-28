#include "frontend/scan/ast.hh"

#include "util/panic.hh"

template <std::derived_from<frontend::scan::ast::t_node> T, typename... ARGS>
T* frontend::scan::ast::t_ast::emplace(ARGS... args) {
    std::optional<T*> ptr = arena.emplace<T, ARGS...>(std::forward(args)...);

    util::panic_assert(ptr.has_value(), "Failed to emplace AST node into arena pool.");

    if constexpr (std::is_same_v<T, t_import_decl>)
        import_nodes.push_back(ptr);

    return ptr.value();
}