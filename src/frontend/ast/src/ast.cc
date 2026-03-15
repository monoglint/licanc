module frontend.ast;

import util;

template <std::derived_from<frontend::ast::Node> T> 
T* post_insert(std::vector<frontend::ast::ImportDecl*>& import_nodes, std::optional<T*> ptr) {
    util::panic_assert(ptr.has_value(), "Failed to emplace AST node into arena pool.");

    if constexpr (std::is_same_v<T, frontend::ast::ImportDecl>)
        import_nodes.push_back(ptr);

    return ptr.value();
}

template <std::derived_from<frontend::ast::Node> T, typename... ARGS>
T* frontend::ast::AST::emplace(ARGS... args) {
    std::optional<T*> ptr = arena.try_emplace<T, ARGS...>(std::forward(args)...);

    return post_insert(import_nodes, ptr);
}

template <std::derived_from<frontend::ast::Node> T>
T* frontend::ast::AST::push(T node) {
    std::optional<T*> ptr = arena.try_push<T>(std::move(node));

    util::panic_assert(ptr.has_value(), "Failed to emplace AST node into arena pool.");

    return post_insert(import_nodes, ptr);
}