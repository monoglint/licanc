#include "frontend/sema/sym.hh"

#include "util/panic.hh"

#include "frontend/ast/ast.hh"

#include "util/panic.hh"

template <std::derived_from<frontend::sema::sym::Sym> T> 
T* post_insert(std::optional<T*> ptr) {
    util::panic_assert(ptr.has_value(), "Failed to emplace symbol into arena pool.");
    
    return ptr.value();
}

template <std::derived_from<frontend::sema::sym::Sym> T, typename... ARGS>
T* frontend::sema::sym::SymTable::emplace(ARGS... args) {
    std::optional<T*> ptr = arena.try_emplace<T, ARGS...>(std::forward(args)...);

    return post_insert(ptr);
}

template <std::derived_from<frontend::sema::sym::Sym> T>
T* frontend::sema::sym::SymTable::push(T sym) {
    std::optional<T*> ptr = arena.try_push<T>(std::move(sym));

    return post_insert(ptr);
}