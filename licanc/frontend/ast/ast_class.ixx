module;

#include <concepts>
#include <optional>

export module frontend.ast:ast_class;

import :ast_nodes;

template <std::derived_from<frontend::ast::Node> T> 
T* post_ast_insert(std::optional<T*> ptr) {
    util::panic_assert(ptr.has_value(), "Failed to emplace node into arena pool.");
    
    return ptr.value();
}

export namespace frontend::ast {
    class AST {
    public:
        AST() { init(); }

        Root* root_ptr;
        
        template <std::derived_from<Node> T, typename... ARGS>
        T* emplace(ARGS... args) {
            std::optional<T*> ptr = arena.try_emplace<T, ARGS...>(std::forward(args)...);

            return post_ast_insert(ptr);
        }

        template <std::derived_from<Node> T>
        T* push(T node) {
            std::optional<T*> ptr = arena.try_push<T>(std::move(node));

            return post_ast_insert(ptr);
        }

        void clear() {
            arena.clear();
            init();
        }

    private:
        // EXTERNAL CALL OF ::clear() IS UNSAFE
        util::Arena<> arena;

        void init() {
            (void)arena.try_emplace<Root>();
        }
    };
}