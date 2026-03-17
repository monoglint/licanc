module;

#include <concepts>
#include <optional>

export module frontend.sema:sym_table_class;

import :syms;

template <std::derived_from<frontend::sema::Sym> T> 
T* post_sym_table_insert(std::optional<T*> ptr) {
    util::panic_assert(ptr.has_value(), "Failed to emplace symbol into arena pool.");
    
    return ptr.value();
}

export namespace frontend::sema {
    class SymTable {
    public:
        SymTable() { init(); }

        Root* root_ptr;
        
        template <std::derived_from<Sym> T, typename... ARGS>
        T* emplace(ARGS... args) {
            std::optional<T*> ptr = arena.try_emplace<T, ARGS...>(std::forward(args)...);

            return post_sym_table_insert(ptr);
        }

        template <std::derived_from<Sym> T>
        T* push(T sym) {
            std::optional<T*> ptr = arena.try_push<T>(std::move(sym));

            return post_sym_table_insert(ptr);
        }

        void clear() {
            arena.clear();
            init();
        }
        
    private:
        util::Arena<> arena;

        void init() {
            (void)arena.try_emplace<Root>();
        }
    };
}