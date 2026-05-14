#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <cassert>

namespace hft {

/**
 * Pre-allocated object pool to avoid heap allocation on the hot path.
 * All objects are constructed once at startup.
 */
template <typename T, std::size_t N>
class ObjectPool {
public:
    ObjectPool() {
        for (std::size_t i = 0; i < N; ++i) {
            freeList_[i] = &objects_[i];
        }
        freeCount_ = N;
    }

    T* acquire() noexcept {
        if (freeCount_ == 0) return nullptr;
        return freeList_[--freeCount_];
    }

    void release(T* obj) noexcept {
        assert(freeCount_ < N);
        freeList_[freeCount_++] = obj;
    }

    std::size_t available() const noexcept { return freeCount_; }

private:
    alignas(64) std::array<T, N>  objects_;
    std::array<T*, N>             freeList_;
    std::size_t                   freeCount_;
};

} // namespace hft
