#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <cstring>

namespace hft {

/**
 * Lock-free Single-Producer Single-Consumer ring buffer.
 * Inspired by the LMAX Disruptor pattern.
 * Cache-line padded to avoid false sharing.
 */
template <typename T, std::size_t N>
class LockfreeRingBuffer {
    static_assert((N & (N - 1)) == 0, "Capacity must be a power of 2");

public:
    LockfreeRingBuffer() : head_(0), tail_(0) {}

    // Producer: enqueue one item. Returns false if full.
    bool push(const T& item) noexcept {
        const uint64_t head = head_.load(std::memory_order_relaxed);
        const uint64_t next = head + 1;
        if (next - tail_.load(std::memory_order_acquire) > N) {
            return false; // buffer full
        }
        buffer_[head & MASK] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer: dequeue one item. Returns false if empty.
    bool pop(T& item) noexcept {
        const uint64_t tail = tail_.load(std::memory_order_relaxed);
        if (head_.load(std::memory_order_acquire) == tail) {
            return false; // buffer empty
        }
        item = buffer_[tail & MASK];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    std::size_t size() const noexcept {
        return head_.load(std::memory_order_acquire) -
               tail_.load(std::memory_order_acquire);
    }

private:
    static constexpr std::size_t MASK = N - 1;

    alignas(64) std::atomic<uint64_t> head_;
    alignas(64) std::atomic<uint64_t> tail_;
    alignas(64) std::array<T, N>      buffer_;
};

} // namespace hft
