#include "util/arena.hh"

template <std::size_t CAPACITY>
template <std::size_t SIZE, std::size_t ALIGN>
void* util::ArenaChunk<CAPACITY>::try_align() {
    std::byte* current_ptr = buffer + offset;
    std::size_t space_left = CAPACITY - offset;
    void* aligned_ptr = current_ptr;

    return std::align(ALIGN, SIZE, aligned_ptr, space_left);
}

template <std::size_t CAPACITY>
template <std::size_t SIZE, std::size_t ALIGN>
void* util::ArenaChunk<CAPACITY>::try_allocate() {
    void* aligned_ptr = try_align<SIZE, ALIGN>();
    
    if (!aligned_ptr)
        return nullptr;

    offset = (static_cast<std::byte*>(aligned_ptr) - buffer) + SIZE;

    return aligned_ptr;
}

template <std::size_t CHUNK_CAPACITY>
template <typename T, typename... ARGS>
std::optional<T*> util::Arena<CHUNK_CAPACITY>::try_emplace(ARGS&&... args) {
    void* ptr = attempt_allocation<sizeof(T), alignof(T)>();

    if (!ptr)
        return std::nullopt;

    T* casted_ptr = static_cast<T*>(ptr);
    new (casted_ptr) T(std::forward<ARGS>(args)...); // NOTE: CAN THROW BAD ALLOC
    register_potential_destructor<T>(casted_ptr);

    return casted_ptr;
}

template <std::size_t CHUNK_CAPACITY>
template <typename T>
std::optional<T*> util::Arena<CHUNK_CAPACITY>::try_push(T obj) {
    void* ptr = attempt_allocation<sizeof(T), alignof(T)>();

    if (!ptr)
        return std::nullopt;

    T* casted_ptr = static_cast<T*>(ptr);
    new (casted_ptr) T(std::move(obj)); // NOTE: CAN THROW BADALLOC
    register_potential_destructor<T>(casted_ptr);

    return casted_ptr;
}

template <std::size_t CHUNK_CAPACITY>
template <std::size_t SIZE, std::size_t ALIGN>
std::optional<void*> util::Arena<CHUNK_CAPACITY>::try_allocate() {
    void* ptr = attempt_allocation<SIZE, ALIGN>();

    if (ptr)
        return ptr;

    return std::nullopt;
}

template <std::size_t CHUNK_CAPACITY>
void util::Arena<CHUNK_CAPACITY>::clear() {
    while (!destructor_wrappers.empty()) {
        DestructorWrapper& wrapper = destructor_wrappers.back();

        wrapper.callback(wrapper.obj_ptr);
        destructor_wrappers.pop_back();
    }

    chunks.resize(1);
    chunks[0].reset_offset();
}

template <std::size_t CHUNK_CAPACITY>
template <typename T>
void util::Arena<CHUNK_CAPACITY>::register_potential_destructor(T* ptr) {
    if constexpr (!std::is_trivially_destructible_v<T>)
        destructor_wrappers.emplace_back(
            [](void* obj_ptr) {
                static_cast<T*>(obj_ptr)->~T();
            },
            ptr
        );
}

template <std::size_t CHUNK_CAPACITY>
template <std::size_t SIZE, std::size_t ALIGN>
void* util::Arena<CHUNK_CAPACITY>::attempt_allocation() {
    static_assert(SIZE <= CHUNK_CAPACITY, "Attempted to allocate a data type too large.");

    void* ptr = chunks.back().template try_allocate<SIZE, ALIGN>();

    if (ptr)
        return ptr;
    // failure occurs if the current chunk ran out of room or if alignment was not possible

    chunks.emplace_back();

    return chunks.back().template try_allocate<SIZE, ALIGN>();
}
