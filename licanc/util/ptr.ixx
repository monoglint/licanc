module;

#include <cassert>
#include <cstddef>

export module util:ptr;

import :panic;

export namespace util {
    // a non-nullable pointer designed to point at one thing specifically
    // warning: THIS IS RAW. THERE IS NO OWNERSHIP OF ANY CONTENT
    template <typename T>
    class FirmPtr {
        FirmPtr(std::nullptr_t) = delete;
        FirmPtr(T* ptr)
        : ptr(ptr)
        {
            util::panic_assert(ptr != nullptr, "Firm pointers can not take in nullptrs.");
        }

        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }
    private:
        T* ptr;
    };

    // a nullable ptr - used for semantic consistency in tandom with firm_ptr
    template <typename T>
    struct OptPtr {
        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }

    private:
        T* ptr;
    };
}