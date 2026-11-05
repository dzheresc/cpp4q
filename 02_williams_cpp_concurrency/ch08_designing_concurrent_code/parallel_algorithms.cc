#include "ch08_designing_concurrent_code/parallel_algorithms.h"

#include <cmath>
#include <stdexcept>
#include <thread>

namespace cpp4q::williams::ch08 {
namespace {

template <typename Counter, typename Get>
std::vector<std::int64_t> CountByBucket(std::span<const std::int32_t> ids,
                                        unsigned num_threads, Get get) {
  if (num_threads == 0) num_threads = 1;
  std::vector<Counter> counters(num_threads);
  {
    std::vector<std::jthread> threads;
    for (unsigned t = 0; t < num_threads; ++t) {
      threads.emplace_back([&, t] {
        auto& mine = get(counters[t]);
        // atomic_ref makes every increment a real read-modify-write in
        // memory, as it would be for a counter other threads read, so the
        // compiler can't keep `mine` in a register for the whole loop.
        for (std::int32_t id : ids) {
          if (static_cast<unsigned>(id) % num_threads == t) {
            std::atomic_ref<std::int64_t>(mine).fetch_add(
                1, std::memory_order_relaxed);
          }
        }
      });
    }
  }
  std::vector<std::int64_t> out;
  out.reserve(num_threads);
  for (auto& c : counters) out.push_back(get(c));
  return out;
}

struct alignas(64) PaddedCounter {
  std::int64_t value = 0;
};

}  // namespace

std::vector<std::int64_t> CountByBucketPacked(std::span<const std::int32_t> ids,
                                              unsigned num_threads) {
  return CountByBucket<std::int64_t>(
      ids, num_threads, [](std::int64_t& c) -> std::int64_t& { return c; });
}

std::vector<std::int64_t> CountByBucketPadded(std::span<const std::int32_t> ids,
                                              unsigned num_threads) {
  return CountByBucket<PaddedCounter>(
      ids, num_threads,
      [](PaddedCounter& c) -> std::int64_t& { return c.value; });
}

double ScenarioVar(std::span<const double> exposures,
                   std::span<const double> scenarios, double confidence,
                   unsigned max_threads) {
  const std::size_t k = exposures.size();
  if (k == 0 || scenarios.size() % k != 0 || scenarios.empty() ||
      !(confidence > 0.0 && confidence < 1.0)) {
    throw std::invalid_argument("ScenarioVar: bad input");
  }
  const std::size_t num_scenarios = scenarios.size() / k;
  std::vector<double> pnl(num_scenarios);
  std::vector<std::size_t> idx(num_scenarios);
  std::iota(idx.begin(), idx.end(), std::size_t{0});
  ParallelForEach(
      idx.begin(), idx.end(),
      [&](std::size_t s) {
        const double* row = scenarios.data() + s * k;
        pnl[s] =
            std::inner_product(exposures.begin(), exposures.end(), row, 0.0);
      },
      max_threads, 256);
  const auto q = static_cast<std::size_t>(
      std::floor((1.0 - confidence) * static_cast<double>(num_scenarios)));
  std::nth_element(pnl.begin(), pnl.begin() + static_cast<std::ptrdiff_t>(q),
                   pnl.end());
  return -pnl[q];
}

}  // namespace cpp4q::williams::ch08
