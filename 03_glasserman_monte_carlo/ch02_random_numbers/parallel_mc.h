// Chapter 2: Generating random numbers and random variables.
//
// Parallel simulation with reproducible streams (Glasserman 2.1.5). The
// paths are cut into fixed-size blocks; block b always draws from
// substream b of the seed's Mrg32k3a, and the per-block statistics are
// merged in block order. The result is therefore bit-identical for any
// thread count, and no two blocks can share random numbers.

#ifndef CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_PARALLEL_MC_H_
#define CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_PARALLEL_MC_H_

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/normal.h"
#include "ch02_random_numbers/rng.h"

namespace cpp4q::glasserman::ch02 {

unsigned DefaultThreadCount();

struct ParallelOptions {
  unsigned threads = 0;  // 0: DefaultThreadCount()
  std::size_t block_size = 8192;
  std::uint64_t seed = 0;
  NormalMethod method = NormalMethod::kInversion;
};

// path(Sampler&) -> double returns one replication.
template <typename PathFn>
ch01::RunningStats ParallelSimulate(PathFn path, std::size_t paths,
                                    const ParallelOptions& opt = {}) {
  const std::size_t block = std::max<std::size_t>(opt.block_size, 1);
  const std::size_t blocks = (paths + block - 1) / block;
  std::vector<ch01::RunningStats> partial(blocks);
  const Mrg32k3a base(opt.seed);
  std::atomic<std::size_t> next{0};
  std::exception_ptr error;
  std::mutex error_mutex;

  auto worker = [&] {
    try {
      for (std::size_t b = next.fetch_add(1); b < blocks;
           b = next.fetch_add(1)) {
        Sampler sampler(base.Substream(b), opt.method);
        const std::size_t end = std::min(paths, (b + 1) * block);
        // Accumulate locally: neighbouring partial[] entries belong to
        // other threads, and updating them in place per path made every
        // Add() a false-sharing cache-line transfer.
        ch01::RunningStats local;
        for (std::size_t i = b * block; i < end; ++i) local.Add(path(sampler));
        partial[b] = local;
      }
    } catch (...) {
      std::lock_guard lock(error_mutex);
      if (!error) error = std::current_exception();
      next.store(blocks);  // stop the other workers early
    }
  };

  const unsigned wanted = opt.threads ? opt.threads : DefaultThreadCount();
  const auto n_threads = static_cast<unsigned>(
      std::min<std::size_t>(wanted, std::max<std::size_t>(blocks, 1)));
  {
    std::vector<std::jthread> pool;
    pool.reserve(n_threads - 1);
    for (unsigned t = 1; t < n_threads; ++t) pool.emplace_back(worker);
    worker();
  }
  if (error) std::rethrow_exception(error);

  ch01::RunningStats total;
  for (const auto& p : partial) total.Merge(p);
  return total;
}

}  // namespace cpp4q::glasserman::ch02

#endif  // CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_PARALLEL_MC_H_
