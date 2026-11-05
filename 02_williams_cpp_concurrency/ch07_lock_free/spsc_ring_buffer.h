// Chapter 7: a wait-free single-producer / single-consumer ring buffer, the
// workhorse between a market-data feed thread and a strategy thread.
//
// The producer owns `tail_`, the consumer owns `head_`; each only reads the
// other's index. Release stores publish slots, acquire loads observe them.
// Each side also caches the last value it saw of the other index, so in
// the common case a push or pop touches no shared cache line at all.

#ifndef CPP4Q_WILLIAMS_CH07_LOCK_FREE_SPSC_RING_BUFFER_H_
#define CPP4Q_WILLIAMS_CH07_LOCK_FREE_SPSC_RING_BUFFER_H_

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace cpp4q::williams::ch07 {

// 64 bytes on every mainstream x86-64 and AArch64 core. Spelled out because
// std::hardware_destructive_interference_size triggers -Winterference-size
// in headers on GCC.
inline constexpr std::size_t kCacheLine = 64;

template <typename T, std::size_t Capacity>
  requires(std::has_single_bit(Capacity) &&
           std::is_nothrow_move_constructible_v<T> &&
           std::is_default_constructible_v<T>)
class SpscRingBuffer {
 public:
  SpscRingBuffer() = default;
  SpscRingBuffer(const SpscRingBuffer&) = delete;
  SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

  // Producer only. Returns false when full.
  bool TryPush(T value) noexcept {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    if (tail - cached_head_ == Capacity) {
      cached_head_ = head_.load(std::memory_order_acquire);
      if (tail - cached_head_ == Capacity) return false;
    }
    slots_[tail & kMask] = std::move(value);
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  // Consumer only. Returns std::nullopt when empty.
  std::optional<T> TryPop() noexcept {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    if (head == cached_tail_) {
      cached_tail_ = tail_.load(std::memory_order_acquire);
      if (head == cached_tail_) return std::nullopt;
    }
    std::optional<T> out(std::move(slots_[head & kMask]));
    head_.store(head + 1, std::memory_order_release);
    return out;
  }

  // Approximate when called concurrently.
  [[nodiscard]] std::size_t size() const noexcept {
    return tail_.load(std::memory_order_acquire) -
           head_.load(std::memory_order_acquire);
  }
  static constexpr std::size_t capacity() { return Capacity; }

 private:
  static constexpr std::size_t kMask = Capacity - 1;

  // Consumer's cache line.
  alignas(kCacheLine) std::atomic<std::size_t> head_{0};
  std::size_t cached_tail_ = 0;
  // Producer's cache line.
  alignas(kCacheLine) std::atomic<std::size_t> tail_{0};
  std::size_t cached_head_ = 0;
  alignas(kCacheLine) std::array<T, Capacity> slots_{};
};

}  // namespace cpp4q::williams::ch07

#endif  // CPP4Q_WILLIAMS_CH07_LOCK_FREE_SPSC_RING_BUFFER_H_
