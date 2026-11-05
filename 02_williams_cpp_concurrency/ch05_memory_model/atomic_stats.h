// Chapter 5: memory orderings in practice.
//
// * TradeCounters: independent statistics counters only need atomicity,
//   not ordering, so every update is memory_order_relaxed.
// * ParallelPnlHistogram: std::atomic_ref (C++20) lets threads update a
//   plain std::vector<std::int64_t> atomically without changing its type.
// * PublishOnce: the release/acquire "message passing" pattern; the
//   consumer that observes ready==true is guaranteed to see the payload.

#ifndef CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_ATOMIC_STATS_H_
#define CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_ATOMIC_STATS_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cpp4q::williams::ch05 {

class TradeCounters {
 public:
  void RecordFill(std::int64_t quantity) noexcept {
    fills_.fetch_add(1, std::memory_order_relaxed);
    volume_.fetch_add(quantity, std::memory_order_relaxed);
  }
  void RecordReject() noexcept {
    rejects_.fetch_add(1, std::memory_order_relaxed);
  }
  [[nodiscard]] std::int64_t fills() const noexcept {
    return fills_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::int64_t volume() const noexcept {
    return volume_.load(std::memory_order_relaxed);
  }
  [[nodiscard]] std::int64_t rejects() const noexcept {
    return rejects_.load(std::memory_order_relaxed);
  }

 private:
  std::atomic<std::int64_t> fills_{0};
  std::atomic<std::int64_t> volume_{0};
  std::atomic<std::int64_t> rejects_{0};
};

// Histogram of `pnl` into `num_bins` equal bins over [lo, hi); values
// outside are clamped to the end bins. Uses `num_threads` threads.
[[nodiscard]] std::vector<std::int64_t> ParallelPnlHistogram(
    std::span<const double> pnl, double lo, double hi, std::size_t num_bins,
    unsigned num_threads);

// Same result, single thread; reference for tests and benchmarks.
[[nodiscard]] std::vector<std::int64_t> PnlHistogram(
    std::span<const double> pnl, double lo, double hi, std::size_t num_bins);

template <typename T>
class PublishOnce {
 public:
  // Call at most once.
  void Publish(T value) {
    value_ = std::move(value);
    ready_.store(true, std::memory_order_release);
    ready_.notify_all();
  }
  [[nodiscard]] const T& Wait() const {
    ready_.wait(false, std::memory_order_acquire);
    return value_;
  }
  [[nodiscard]] bool ready() const {
    return ready_.load(std::memory_order_acquire);
  }

 private:
  T value_{};
  std::atomic<bool> ready_{false};
};

}  // namespace cpp4q::williams::ch05

#endif  // CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_ATOMIC_STATS_H_
