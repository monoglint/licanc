#include "frontend/sema/sym.hh"

#include "util/panic.hh"

template <std::derived_from<frontend::sema::sym::t_sym> T, typename... ARGS>
inline T* frontend::sema::sym::t_sym_table::emplace(ARGS... args) {
    std::optional<T*> ptr = arena.emplace<T, ARGS...>(std::forward(args)...);

    util::panic_assert(ptr.has_value(), "Failed to emplace symbol into arena pool.");

    return ptr.value();
}