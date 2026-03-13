#pragma once

#include <cstddef>
#include <memory>
#include <deque>
#include <vector>
#include <optional>

// notes for future improvements:
// - vector of destructors could maybe be backed into the arena itself (7 march 2026)

namespace util {
    template <std::size_t CAPACITY>
    struct ArenaChunk {
        ArenaChunk()
            : buffer(static_cast<std::byte*>(::operator new(CAPACITY))), offset(0) {}

        ~ArenaChunk() {
            ::operator delete(buffer);
        }

        ArenaChunk(const ArenaChunk&) = delete;
        ArenaChunk(ArenaChunk&&) = delete;

        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        void* try_align() {
            std::byte* current_ptr = buffer + offset;
            std::size_t space_left = CAPACITY - offset;
            void* aligned_ptr = current_ptr;

            return std::align(ALIGN, SIZE, aligned_ptr, space_left);
        }
        
        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        void* try_allocate() {
            void* aligned_ptr = try_align<SIZE, ALIGN>();
            
            if (!aligned_ptr)
                return nullptr;

            offset = (static_cast<std::byte*>(aligned_ptr) - buffer) + SIZE;

            return aligned_ptr;
        }

        void reset_offset() {
            offset = 0;
        }
    private:
        std::byte* buffer;
        std::size_t offset;
    };

    /*
    
    WARNING: DESTRUCTORS WILL NOT CALL WHEN ARENA IS CLEARED
    
    */
    template <std::size_t CHUNK_CAPACITY = 4096>
    class Arena {
    public:
        explicit Arena()
            : chunks(1)
        {};

        ~Arena() {
            clear();
        }

        Arena(const Arena&) = delete;
        Arena(Arena&&) = delete;
        Arena& operator=(const Arena&) = delete;
        Arena& operator=(Arena&&) = delete;

        template <typename T, typename... ARGS>
        [[nodiscard]]
        std::optional<T*> try_emplace(ARGS&&... args) {
            void* ptr = attempt_allocation<sizeof(T), alignof(T)>();

            if (!ptr)
                return std::nullopt;

            T* casted_ptr = static_cast<T*>(ptr);
            new (casted_ptr) T(std::forward<ARGS>(args)...); // NOTE: CAN THROW BAD ALLOC
            register_potential_destructor<T>(casted_ptr);

            return casted_ptr;
        }

        template <typename T>
        [[nodiscard]]
        std::optional<T*> try_push(T obj) {
            void* ptr = attempt_allocation<sizeof(T), alignof(T)>();

            if (!ptr)
                return std::nullopt;

            T* casted_ptr = static_cast<T*>(ptr);
            new (casted_ptr) T(std::move(obj)); // NOTE: CAN THROW BADALLOC
            register_potential_destructor<T>(casted_ptr);

            return casted_ptr;
        }

        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        std::optional<void*> try_allocate() {
            void* ptr = attempt_allocation<SIZE, ALIGN>();

            if (ptr)
                return ptr;

            return std::nullopt;
        }
 
        void clear() {
            while (!destructor_wrappers.empty()) {
                DestructorWrapper& wrapper = destructor_wrappers.back();

                wrapper.callback(wrapper.obj_ptr);
                destructor_wrappers.pop_back();
            }

            chunks.resize(1);
            chunks[0].reset_offset();
        }
    private:
        struct DestructorWrapper {
            void(*callback)(void*);
            void* obj_ptr;
        };

        std::deque<ArenaChunk<CHUNK_CAPACITY>> chunks;
        std::vector<DestructorWrapper> destructor_wrappers;

        template <typename T>
        void register_potential_destructor(T* ptr) {
            if constexpr (!std::is_trivially_destructible_v<T>)
                destructor_wrappers.emplace_back(
                    [](void* obj_ptr) {
                        static_cast<T*>(obj_ptr)->~T();
                    },
                    ptr
                );
        }

        // CAN RETURN NULLPTR
        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        void* attempt_allocation() {
            static_assert(SIZE <= CHUNK_CAPACITY, "Attempted to allocate a data type too large.");

            void* ptr = chunks.back().template try_allocate<SIZE, ALIGN>();

            if (ptr)
                return ptr;
            // failure occurs if the current chunk ran out of room or if alignment was not possible

            chunks.emplace_back();

            return chunks.back().template try_allocate<SIZE, ALIGN>();
        }

    };
}