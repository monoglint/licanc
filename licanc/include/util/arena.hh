#pragma once

#include <cstddef>
#include <new>
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
        void* try_align();
        
        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        void* try_allocate();

        inline void reset_offset() {
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
        std::optional<T*> try_emplace(ARGS&&... args);

        template <typename T>
        [[nodiscard]]
        std::optional<T*> try_push(T obj);

        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        std::optional<void*> try_allocate();

        void clear();
    private:
        struct DestructorWrapper {
            void(*callback)(void*);
            void* obj_ptr;
        };

        std::deque<ArenaChunk<CHUNK_CAPACITY>> chunks;
        std::vector<DestructorWrapper> destructor_wrappers;

        template <typename T>
        void register_potential_destructor(T* ptr);

        // CAN RETURN NULLPTR
        template <std::size_t SIZE, std::size_t ALIGN>
        [[nodiscard]]
        void* attempt_allocation();
    };
}