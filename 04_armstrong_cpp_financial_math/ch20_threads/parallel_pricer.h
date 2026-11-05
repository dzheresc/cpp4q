// Chapter 20: Threads.
//
// Monte Carlo is embarrassingly parallel, but two things need care: each
// thread needs its own random number generator (a shared one is a data
// race, and a mutex around it serialises everything), and the result
// should not depend on how many threads ran it. Here the scenarios are
// cut into fixed blocks, each with its own generator seeded from
// (seed, block index); threads claim blocks from an atomic counter; and
// block results are combined in block order. The answer is bit-identical
// for 1 or 64 threads.
//
// Two alternative shapes are shown as well: std::async with futures (one
// task per thread, each owning a contiguous range of blocks), and a
// mutex-protected shared accumulator.

#ifndef CPP4Q_ARMSTRONG_CH20_THREADS_PARALLEL_PRICER_H_
#define CPP4Q_ARMSTRONG_CH20_THREADS_PARALLEL_PRICER_H_

#include <cstdint>

#include "ch12_sophisticated_classes/path_dependent.h"

namespace cpp4q::armstrong::ch20 {

using ch10::BlackScholesModel;
using ch10::ContinuousTimeOption;
using ch10::Estimate;

struct ParallelSettings {
  int n_scenarios = 100'000;
  int n_steps = 12;        // monitoring dates for path-dependent options
  int n_threads = 4;
  int n_blocks = 64;       // fixed work units; results depend only on this
  std::uint64_t seed = 1;
};

// Atomic block counter + std::jthread. Deterministic in (settings minus
// n_threads). `option` must be safe to call concurrently (const and
// stateless, as every option in this book is).
Estimate PriceParallel(const ContinuousTimeOption& option,
                       const BlackScholesModel& model,
                       const ParallelSettings& settings);

// std::async: n_threads tasks over contiguous block ranges, joined
// through futures. Same blocks and seeds, so the same answer as
// PriceParallel.
Estimate PriceWithAsync(const ContinuousTimeOption& option,
                        const BlackScholesModel& model,
                        const ParallelSettings& settings);

// Each thread merges its blocks into one shared accumulator under a
// mutex. Correct, but the combination order (and so the last few bits)
// depends on scheduling.
Estimate PriceWithMutex(const ContinuousTimeOption& option,
                        const BlackScholesModel& model,
                        const ParallelSettings& settings);

}  // namespace cpp4q::armstrong::ch20

#endif  // CPP4Q_ARMSTRONG_CH20_THREADS_PARALLEL_PRICER_H_
