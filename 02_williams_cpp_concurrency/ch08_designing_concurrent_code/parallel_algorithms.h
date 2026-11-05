// Chapter 8: Designing concurrent code.
//
// Hand-written parallel algorithms that show the chapter's design points:
// how to divide work (contiguous blocks), exception safety (JoiningThread
// owners plus futures to carry exceptions), early termination
// (ParallelFind), multi-pass algorithms (ParallelInclusiveScan), and data
// layout (false sharing, see CountByBucket*).

#ifndef CPP4Q_WILLIAMS_CH08_DESIGNING_CONCURRENT_CODE_PARALLEL_ALGORITHMS_H_
#define CPP4Q_WILLIAMS_CH08_DESIGNING_CONCURRENT_CODE_PARALLEL_ALGORITHMS_H_

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <future>
#include <iterator>
#include <numeric>
#include <span>
#include <vector>

#include "ch02_managing_threads/parallel_accumulate.h"
#include "ch02_managing_threads/thread_guards.h"

namespace cpp4q::williams::ch08 {

using ch02::ChooseThreadCount;
using ch02::JoiningThread;

// Applies f to every element. Exceptions from any block propagate to the
// caller (the first block's exception wins) after all threads finish.
template <std::random_access_iterator It, typename F>
void ParallelForEach(It first, It last, F f, unsigned max_threads = 0,
                     std::size_t min_per_thread = 1024) {
  const auto length = static_cast<std::size_t>(last - first);
  if (length == 0) return;
  const unsigned n = ChooseThreadCount(length, min_per_thread, max_threads);
  std::vector<std::future<void>> results;
  results.reserve(n - 1);
  {
    std::vector<JoiningThread> threads;
    threads.reserve(n - 1);
    for (unsigned t = 0; t + 1 < n; ++t) {
      const It b = first + static_cast<std::ptrdiff_t>(length * t / n);
      const It e = first + static_cast<std::ptrdiff_t>(length * (t + 1) / n);
      std::packaged_task<void()> task([b, e, &f] { std::for_each(b, e, f); });
      results.push_back(task.get_future());
      threads.emplace_back(std::move(task));
    }
    std::for_each(first + static_cast<std::ptrdiff_t>(length * (n - 1) / n),
                  last, f);
  }
  for (auto& r : results) r.get();
}

// Returns the FIRST element satisfying pred, like std::find_if, but
// searches blocks in parallel. A block stops as soon as its position passes
// the best match found so far by any thread, so the search ends early when
// a match appears near the front.
template <std::random_access_iterator It, typename Pred>
It ParallelFind(It first, It last, Pred pred, unsigned max_threads = 0,
                std::size_t min_per_thread = 4096) {
  const auto length = static_cast<std::size_t>(last - first);
  if (length == 0) return last;
  const unsigned n = ChooseThreadCount(length, min_per_thread, max_threads);
  std::atomic<std::size_t> best{length};
  auto search = [&](std::size_t b, std::size_t e) {
    // Check the shared bound every 256 items: cheap, and good enough.
    for (std::size_t i = b; i < e; ++i) {
      if ((i & 255U) == 0 && i >= best.load(std::memory_order_relaxed)) return;
      if (pred(first[static_cast<std::ptrdiff_t>(i)])) {
        std::size_t cur = best.load(std::memory_order_relaxed);
        while (i < cur &&
               !best.compare_exchange_weak(cur, i, std::memory_order_relaxed)) {
        }
        return;
      }
    }
  };
  {
    std::vector<JoiningThread> threads;
    threads.reserve(n - 1);
    for (unsigned t = 1; t < n; ++t) {
      threads.emplace_back(search, length * t / n, length * (t + 1) / n);
    }
    search(0, length / n);
  }
  return first + static_cast<std::ptrdiff_t>(best.load());
}

// Prefix sum in three phases: each thread scans its block, the block totals
// are scanned serially (n values), then each thread adds its offset.
template <typename T>
void ParallelInclusiveScan(std::span<const T> in, std::span<T> out,
                           unsigned max_threads = 0,
                           std::size_t min_per_thread = 16'384) {
  const std::size_t length = std::min(in.size(), out.size());
  if (length == 0) return;
  const unsigned n = ChooseThreadCount(length, min_per_thread, max_threads);
  std::vector<T> block_total(n, T{});
  auto bounds = [&](unsigned t) {
    return std::pair{length * t / n, length * (t + 1) / n};
  };
  auto run = [&](auto&& body) {
    std::vector<JoiningThread> threads;
    threads.reserve(n - 1);
    for (unsigned t = 1; t < n; ++t) threads.emplace_back(body, t);
    body(0U);
  };
  run([&](unsigned t) {
    const auto [b, e] = bounds(t);
    std::inclusive_scan(in.begin() + static_cast<std::ptrdiff_t>(b),
                        in.begin() + static_cast<std::ptrdiff_t>(e),
                        out.begin() + static_cast<std::ptrdiff_t>(b));
    block_total[t] = out[e - 1];
  });
  std::exclusive_scan(block_total.begin(), block_total.end(),
                      block_total.begin(), T{});
  run([&](unsigned t) {
    if (t == 0) return;
    const auto [b, e] = bounds(t);
    const T offset = block_total[t];
    for (std::size_t i = b; i < e; ++i) out[i] += offset;
  });
}

// Counts trades per thread-owned bucket. Each thread increments only its
// own counter, so there is no data race either way; the difference is
// layout. In the Packed version neighbouring counters share a cache line
// and every increment invalidates the other cores' copies (false sharing).
// The Padded version gives each counter its own line.
std::vector<std::int64_t> CountByBucketPacked(std::span<const std::int32_t> ids,
                                              unsigned num_threads);
std::vector<std::int64_t> CountByBucketPadded(std::span<const std::int32_t> ids,
                                              unsigned num_threads);

// Scenario-based VaR: PnL of a linear portfolio (`exposures`) under each
// of `scenarios` (row-major, num_scenarios x exposures.size()) computed in
// parallel, then the (1-confidence) loss quantile.
double ScenarioVar(std::span<const double> exposures,
                   std::span<const double> scenarios, double confidence,
                   unsigned max_threads = 0);

}  // namespace cpp4q::williams::ch08

#endif  // CPP4Q_WILLIAMS_CH08_DESIGNING_CONCURRENT_CODE_PARALLEL_ALGORITHMS_H_
