module;

#include <concepts>

export module frontend.ast:ast_class;

import :ast_nodes;

export namespace frontend::ast {
    // A wrapper for util::arena that holds AST nodes.
    class AST {
    public:
        AST() { init(); }

        Root* root_ptr;
        
        template <std::derived_from<Node> T, typename... ARGS>
        T* emplace(ARGS... args) {
            util::OptPtr<T> ptr = arena.try_emplace<T>(std::forward(args)...);

            return post_ast_insert(ptr);
        }

        template <std::derived_from<Node> T>
        T* push(T node) {
            util::OptPtr<T> ptr = arena.try_push<T>(std::move(node));

            return post_ast_insert(ptr);
        }

        // external access to ::init()
        void clear() {
            init();
        }

        [[nodiscard]]
        std::size_t get_node_count() const {
            return node_count;
        }

    private:
        // EXTERNAL CALL OF Arena::clear() IS UNSAFE, use AST:clear()
        util::Arena<> arena;

        // this variable is used to generate the numeric id for the next appended AST node
        std::size_t node_count = 0;

        void init() {
            arena.clear();
            node_count = 0;
            (void)arena.try_emplace<Root>(NodeInitParams(util::Span(), NodeId(0)));
        }

        template <std::derived_from<frontend::ast::Node> T> 
        T* post_ast_insert(util::OptPtr<T> ptr) {
            util::panic_assert(ptr.has_value(), "Failed to emplace node into arena pool.");
            
            node_count++;
            return ptr.value();
        }
    };
}