#pragma once

#include <cstddef>
#include <new>
#include <memory>
#include <deque>

namespace util {
    template <std::size_t CAPACITY>
    struct t_arena_chunk {
        t_arena_chunk()
            : buffer(static_cast<std::byte*>(::operator new(CAPACITY))), offset(0) {}

        ~t_arena_chunk() {
            ::operator delete(buffer); 
        }

        t_arena_chunk(const t_arena_chunk&) = delete;
        t_arena_chunk(t_arena_chunk&&) = delete;

        template <std::size_t ALIGN, std::size_t SIZE>
        constexpr inline bool can_allocate() {
            std::byte* current_ptr = buffer + offset;
            std::size_t space_left = CAPACITY - offset;
            void* aligned_ptr = current_ptr;

            return std::align(ALIGN, SIZE, aligned_ptr, space_left) != nullptr;
        }

        template <typename T, typename... ARGS>
        [[nodiscard]]
        constexpr inline T* allocate(ARGS&&... args) {
            if (!can_allocate<alignof(T), sizeof(T)>())
                return nullptr;
            
            std::byte* current_ptr = buffer + offset;
            void* aligned_ptr = current_ptr;

            offset += sizeof(T);

            return new (aligned_ptr) T(std::forward<ARGS>(args)...);
        }

        std::byte* buffer;
        std::size_t offset;
    };

    /*
    
    WARNING: ONLY PASS AGREGATE CLASSES INTO THIS STRUCT.
    DESTRUCTORS WILL NOT CALL WHEN ARENA IS CLEARED
    
    */
    template <std::size_t CHUNK_CAPACITY = 4096>
    struct t_arena {
        explicit t_arena()
            : chunks(1) {};

        t_arena(const t_arena&) = delete;
        t_arena(t_arena&&) = delete;
        t_arena& operator=(const t_arena&) = delete;
        t_arena& operator=(t_arena&&) = delete;

        template <typename T, typename... ARGS>
        [[nodiscard]]
        constexpr inline T* emplace(ARGS&&... args) {
            T* ptr = attempt_allocation<T, ARGS...>(std::forward<ARGS>(args)...);

            if (ptr)
                return ptr;

            chunks.emplace_back();
            return attempt_allocation<T, ARGS...>(std::forward<ARGS>(args)...);
        }

        template <typename T>
        [[nodiscard]]
        constexpr inline T* push(T&& obj) {
            return emplace<T, T>(std::move(obj));
        }
    private: 
        std::deque<t_arena_chunk<CHUNK_CAPACITY>> chunks;

        template <typename T, typename... ARGS>
        constexpr inline T* attempt_allocation(ARGS&&... args) {
            static_assert(sizeof(T) <= CHUNK_CAPACITY, "Attempted to allocate a data type too large.");
            return chunks.back().template allocate<T>(std::forward<ARGS>(args)...);
        }
    };
}