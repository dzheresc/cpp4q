// Chapter 5: a sequence lock for a single-writer, many-reader snapshot,
// e.g. top of book.
//
// The writer bumps an odd sequence number, writes the payload, then makes
// the sequence even again. A reader copies the payload between two loads
// of the sequence and retries if they differ or are odd. Readers never
// block the writer.
//
// To stay free of data races in the C++ memory model, the payload is
// stored as relaxed atomic 64-bit words, with fences supplying the
// ordering (Boehm, "Can seqlocks get along with programming language
// memory models?", 2012). The payload type must be trivially copyable.

#ifndef CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_SEQ_LOCK_H_
#define CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_SEQ_LOCK_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace cpp4q::williams::ch05 {

template <typename T>
  requires std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>
class SeqLock {
 public:
  SeqLock() { Store(T{}); }
  explicit SeqLock(const T& initial) { Store(initial); }

  // Single writer only.
  void Store(const T& value) noexcept {
    std::array<std::uint64_t, kWords> buf{};
    std::memcpy(buf.data(), &value, sizeof(T));
    const std::uint64_t s = seq_.load(std::memory_order_relaxed);
    seq_.store(s + 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    for (std::size_t i = 0; i < kWords; ++i) {
      words_[i].store(buf[i], std::memory_order_relaxed);
    }
    seq_.store(s + 2, std::memory_order_release);
  }

  // Wait-free for the writer, lock-free for readers.
  [[nodiscard]] T Load() const noexcept {
    std::array<std::uint64_t, kWords> buf{};
    for (;;) {
      const std::uint64_t s0 = seq_.load(std::memory_order_acquire);
      if (s0 & 1U) continue;  // write in progress
      for (std::size_t i = 0; i < kWords; ++i) {
        buf[i] = words_[i].load(std::memory_order_relaxed);
      }
      std::atomic_thread_fence(std::memory_order_acquire);
      if (seq_.load(std::memory_order_relaxed) == s0) break;
    }
    T out;
    std::memcpy(static_cast<void*>(&out), buf.data(), sizeof(T));
    return out;
  }

  [[nodiscard]] std::uint64_t version() const noexcept {
    return seq_.load(std::memory_order_acquire) / 2;
  }

 private:
  static constexpr std::size_t kWords = (sizeof(T) + 7) / 8;
  std::atomic<std::uint64_t> seq_{0};
  std::array<std::atomic<std::uint64_t>, kWords> words_{};
};

struct TopOfBook {
  double bid = 0.0;
  double ask = 0.0;
  std::int64_t bid_size = 0;
  std::int64_t ask_size = 0;
};

}  // namespace cpp4q::williams::ch05

#endif  // CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_SEQ_LOCK_H_
