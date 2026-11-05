// Chapter 4: one-off events with futures, std::packaged_task, std::promise,
// functional-style parallelism with std::async, and the C++20 latch and
// barrier.

#ifndef CPP4Q_WILLIAMS_CH04_SYNCHRONIZING_FUTURES_H_
#define CPP4Q_WILLIAMS_CH04_SYNCHRONIZING_FUTURES_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <thread>
#include <utility>
#include <vector>

#include "ch04_synchronizing/thread_safe_queue.h"

namespace cpp4q::williams::ch04 {

// A background pricing thread that runs submitted jobs in FIFO order. Each
// job is wrapped in a std::packaged_task, so its result or exception comes
// back through the returned future.
class PricingService {
 public:
  PricingService();
  ~PricingService();  // drains outstanding jobs, then joins
  PricingService(const PricingService&) = delete;
  PricingService& operator=(const PricingService&) = delete;

  std::future<double> Submit(std::function<double()> job);

 private:
  ThreadSafeQueue<std::packaged_task<double()>> jobs_;
  std::jthread worker_;
};

// Historical VaR computed on a new thread; the result (or a
// std::invalid_argument for bad input) is delivered via std::promise.
std::future<double> HistoricalVarAsync(std::vector<double> pnl,
                                       double confidence);

// Sorts with std::async: the lower partition is sorted on another thread
// while this one sorts the upper partition. Below `serial_cutoff` items, or
// once `depth` reaches zero, it falls back to std::sort so the number of
// threads stays bounded.
template <typename T, typename Compare = std::less<>>
void ParallelQuickSort(std::vector<T>& v, Compare comp = {},
                       std::size_t serial_cutoff = 4096, int depth = 4) {
  struct Impl {
    static void Run(T* first, T* last, Compare& comp, std::size_t cutoff,
                    int depth) {
      const auto n = static_cast<std::size_t>(last - first);
      if (n <= cutoff || depth <= 0) {
        std::sort(first, last, comp);
        return;
      }
      // Median of three as pivot, then a three-way split so runs of equal
      // prices (common in trade data) don't degrade to O(n^2).
      const T a = first[0], b = first[n / 2], c = last[-1];
      const T pivot = comp(a, b) ? (comp(b, c) ? b : (comp(a, c) ? c : a))
                                 : (comp(a, c) ? a : (comp(b, c) ? c : b));
      T* mid1 = std::partition(first, last,
                               [&](const T& x) { return comp(x, pivot); });
      T* mid2 = std::partition(mid1, last,
                               [&](const T& x) { return !comp(pivot, x); });
      auto lower = std::async(std::launch::async, [=, &comp] {
        Run(first, mid1, comp, cutoff, depth - 1);
      });
      Run(mid2, last, comp, cutoff, depth - 1);
      lower.get();  // rethrows anything the other half threw
    }
  };
  if (v.empty()) return;
  Impl::Run(v.data(), v.data() + v.size(), comp, serial_cutoff, depth);
}

// Time-stepped GBM simulation in lock-step. Each worker owns a slice of the
// paths; at the end of every step a std::barrier's completion function
// (run by one thread while the rest wait) records the cross-sectional mean
// spot. A std::latch holds all workers at the start line. Deterministic
// for a given (num_paths, num_workers, seed).
std::vector<double> LockstepMeanSpot(std::size_t num_paths,
                                     std::size_t num_steps, double spot,
                                     double vol, double dt,
                                     unsigned num_workers, std::uint64_t seed);

}  // namespace cpp4q::williams::ch04

#endif  // CPP4Q_WILLIAMS_CH04_SYNCHRONIZING_FUTURES_H_
