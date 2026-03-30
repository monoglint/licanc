module;

#include <cassert>
#include <cstddef>

export module util:ptr;

import :panic;

export namespace util {
    // A pointer that is guaranteed to not point to nullptr.
    template <typename T>
    class FirmPtr {
    public:
        FirmPtr(std::nullptr_t) = delete;
        FirmPtr(T* ptr)
        : ptr(ptr)
        {
            util::panic_assert(ptr != nullptr, "Firm pointers can not take in nullptrs.");
        }

        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }

        auto operator<=>(const FirmPtr&) const = default;
    private:
        T* ptr;
    };

    // A 
    template <typename T>
    class OptPtr {
    public:
        OptPtr(T* ptr)
            : ptr(ptr)
        {}

        OptPtr()
            : ptr(nullptr)
        {}

        bool has_value() const {
            return ptr != nullptr;
        }

        T& operator*() const { return *ptr; }
        T* operator->() const { return ptr; }

    private:
        T* ptr;
    };
}