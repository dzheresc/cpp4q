// Chapter 2: choosing the number of threads at run time and handing each a
// block of the input.
//
// ParallelAccumulate splits [first, last) into contiguous blocks, one per
// thread, sums each block into its own slot and folds the partial sums in
// order. The calling thread takes the last block itself. Thread count is
// capped so that every thread gets at least `min_per_thread` items, because
// starting a thread costs tens of microseconds.

#ifndef CPP4Q_WILLIAMS_CH02_MANAGING_THREADS_PARALLEL_ACCUMULATE_H_
#define CPP4Q_WILLIAMS_CH02_MANAGING_THREADS_PARALLEL_ACCUMULATE_H_

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <span>
#include <thread>
#include <vector>

#include "ch02_managing_threads/thread_guards.h"

namespace cpp4q::williams::ch02 {

// Number of worker threads to use for `num_items` work items:
// min(hardware threads, ceil(num_items / min_per_thread)), at least 1.
[[nodiscard]] unsigned ChooseThreadCount(std::size_t num_items,
                                         std::size_t min_per_thread,
                                         unsigned max_threads = 0);

template <std::random_access_iterator It, typename T>
T ParallelAccumulate(It first, It last, T init, std::size_t min_per_thread = 25,
                     unsigned max_threads = 0) {
  const auto length = static_cast<std::size_t>(std::distance(first, last));
  if (length == 0) return init;
  const unsigned num_threads =
      ChooseThreadCount(length, min_per_thread, max_threads);
  const std::size_t block = length / num_threads;

  std::vector<T> partial(num_threads, T{});
  {
    std::vector<JoiningThread> workers;
    workers.reserve(num_threads - 1);
    It block_start = first;
    for (unsigned i = 0; i + 1 < num_threads; ++i) {
      It block_end = block_start + static_cast<std::ptrdiff_t>(block);
      workers.emplace_back([block_start, block_end, &slot = partial[i]] {
        slot = std::accumulate(block_start, block_end, T{});
      });
      block_start = block_end;
    }
    partial.back() = std::accumulate(block_start, last, T{});
  }  // JoiningThread destructors join here, even if the last block throws.
  return std::accumulate(partial.begin(), partial.end(), init);
}

struct Position {
  double quantity = 0.0;
  double entry_price = 0.0;
  double mark_price = 0.0;
};

// Mark-to-market PnL summed over all positions with ParallelAccumulate.
[[nodiscard]] double PortfolioPnl(std::span<const Position> positions,
                                  unsigned max_threads = 0);

}  // namespace cpp4q::williams::ch02

#endif  // CPP4Q_WILLIAMS_CH02_MANAGING_THREADS_PARALLEL_ACCUMULATE_H_
