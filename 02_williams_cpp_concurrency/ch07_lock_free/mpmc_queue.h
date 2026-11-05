// Chapter 7: a bounded lock-free multi-producer / multi-consumer queue
// (the sequence-numbered ring popularised by Dmitry Vyukov).
//
// Every slot carries a sequence number. A producer may write slot i only
// when seq == position, and publishes by setting seq = position + 1; a
// consumer may read it only when seq == position + 1, and frees it with
// seq = position + Capacity. Producers and consumers each claim positions
// with a CAS on their own counter, so there is no ABA problem and no
// memory reclamation to worry about.

#ifndef CPP4Q_WILLIAMS_CH07_LOCK_FREE_MPMC_QUEUE_H_
#define CPP4Q_WILLIAMS_CH07_LOCK_FREE_MPMC_QUEUE_H_

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "ch07_lock_free/spsc_ring_buffer.h"

namespace cpp4q::williams::ch07 {

template <typename T>
  requires std::is_nothrow_move_constructible_v<T>
class MpmcBoundedQueue {
 public:
  explicit MpmcBoundedQueue(std::size_t capacity)
      : mask_(capacity - 1), cells_(std::make_unique<Cell[]>(capacity)) {
    if (capacity < 2 || !std::has_single_bit(capacity)) {
      throw std::invalid_argument("MpmcBoundedQueue: capacity must be 2^k");
    }
    for (std::size_t i = 0; i < capacity; ++i) {
      cells_[i].seq.store(i, std::memory_order_relaxed);
    }
  }
  MpmcBoundedQueue(const MpmcBoundedQueue&) = delete;
  MpmcBoundedQueue& operator=(const MpmcBoundedQueue&) = delete;
  ~MpmcBoundedQueue() {
    while (TryPop()) {
    }
  }

  bool TryPush(T value) noexcept {
    std::size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
    Cell* cell;
    for (;;) {
      cell = &cells_[pos & mask_];
      const std::size_t seq = cell->seq.load(std::memory_order_acquire);
      const auto diff =
          static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
      if (diff == 0) {
        if (enqueue_pos_.compare_exchange_weak(pos, pos + 1,
                                               std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0) {
        return false;  // full
      } else {
        pos = enqueue_pos_.load(std::memory_order_relaxed);
      }
    }
    ::new (cell->storage) T(std::move(value));
    cell->seq.store(pos + 1, std::memory_order_release);
    return true;
  }

  std::optional<T> TryPop() noexcept {
    std::size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
    Cell* cell;
    for (;;) {
      cell = &cells_[pos & mask_];
      const std::size_t seq = cell->seq.load(std::memory_order_acquire);
      const auto diff =
          static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
      if (diff == 0) {
        if (dequeue_pos_.compare_exchange_weak(pos, pos + 1,
                                               std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0) {
        return std::nullopt;  // empty
      } else {
        pos = dequeue_pos_.load(std::memory_order_relaxed);
      }
    }
    T* item = std::launder(reinterpret_cast<T*>(cell->storage));
    std::optional<T> out(std::move(*item));
    item->~T();
    cell->seq.store(pos + mask_ + 1, std::memory_order_release);
    return out;
  }

  [[nodiscard]] std::size_t capacity() const { return mask_ + 1; }

 private:
  struct alignas(kCacheLine) Cell {
    std::atomic<std::size_t> seq{0};
    alignas(T) unsigned char storage[sizeof(T)];
  };

  const std::size_t mask_;
  std::unique_ptr<Cell[]> cells_;
  alignas(kCacheLine) std::atomic<std::size_t> enqueue_pos_{0};
  alignas(kCacheLine) std::atomic<std::size_t> dequeue_pos_{0};
};

}  // namespace cpp4q::williams::ch07

#endif  // CPP4Q_WILLIAMS_CH07_LOCK_FREE_MPMC_QUEUE_H_
