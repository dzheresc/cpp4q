#include "ch20_threads/parallel_pricer.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <future>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ch09_monte_carlo/monte_carlo.h"

namespace cpp4q::armstrong::ch20 {
namespace {

// Welford accumulator with Chan's merge, so block results combine exactly.
struct Stats {
  double n = 0.0;
  double mean = 0.0;
  double m2 = 0.0;

  void Add(double x) {
    n += 1.0;
    const double d = x - mean;
    mean += d / n;
    m2 += d * (x - mean);
  }
  void Merge(const Stats& o) {
    if (o.n == 0) return;
    if (n == 0) {
      *this = o;
      return;
    }
    const double total = n + o.n;
    const double d = o.mean - mean;
    mean += d * o.n / total;
    m2 += o.m2 + d * d * n * o.n / total;
    n = total;
  }
};

void Validate(const ParallelSettings& s) {
  if (s.n_scenarios < 2 || s.n_steps < 1 || s.n_threads < 1 ||
      s.n_blocks < 1) {
    throw std::invalid_argument("invalid ParallelSettings");
  }
}

// Number of scenarios in block b: n_scenarios split as evenly as possible.
int BlockSize(const ParallelSettings& s, int b) {
  return s.n_scenarios / s.n_blocks + (b < s.n_scenarios % s.n_blocks ? 1 : 0);
}

Stats SimulateBlock(const ContinuousTimeOption& option,
                    const BlackScholesModel& model,
                    const ParallelSettings& s, int block) {
  std::seed_seq seq{static_cast<std::uint32_t>(s.seed),
                    static_cast<std::uint32_t>(s.seed >> 32),
                    static_cast<std::uint32_t>(block)};
  ch09::Rng rng(seq);
  const int steps = option.IsPathDependent() ? s.n_steps : 1;
  Stats stats;
  for (int i = BlockSize(s, block); i > 0; --i) {
    const std::vector<double> path = ch09::GeneratePricePath(
        model, option.maturity(), steps, ch09::Measure::kRiskNeutral, rng);
    stats.Add(option.Payoff(path));
  }
  return stats;
}

Estimate ToEstimate(const Stats& total, const ContinuousTimeOption& option,
                    const BlackScholesModel& model) {
  const double df =
      std::exp(-model.risk_free_rate * (option.maturity() - model.date));
  return {df * total.mean, df * std::sqrt(total.m2 / (total.n - 1) / total.n)};
}

}  // namespace

Estimate PriceParallel(const ContinuousTimeOption& option,
                       const BlackScholesModel& model,
                       const ParallelSettings& s) {
  Validate(s);
  std::vector<Stats> blocks(static_cast<std::size_t>(s.n_blocks));
  std::atomic<int> next{0};
  {
    std::vector<std::jthread> workers;
    const int n_workers = std::min(s.n_threads, s.n_blocks);
    for (int t = 0; t < n_workers; ++t) {
      workers.emplace_back([&] {
        for (int b = next.fetch_add(1, std::memory_order_relaxed);
             b < s.n_blocks; b = next.fetch_add(1, std::memory_order_relaxed)) {
          blocks[static_cast<std::size_t>(b)] =
              SimulateBlock(option, model, s, b);
        }
      });
    }
  }  // jthreads join here; their writes happen-before the reads below
  Stats total;
  for (const Stats& b : blocks) total.Merge(b);
  return ToEstimate(total, option, model);
}

Estimate PriceWithAsync(const ContinuousTimeOption& option,
                        const BlackScholesModel& model,
                        const ParallelSettings& s) {
  Validate(s);
  const int n_tasks = std::min(s.n_threads, s.n_blocks);
  std::vector<std::future<std::vector<Stats>>> futures;
  for (int t = 0; t < n_tasks; ++t) {
    const int first = s.n_blocks * t / n_tasks;
    const int last = s.n_blocks * (t + 1) / n_tasks;
    futures.push_back(std::async(std::launch::async, [&, first, last] {
      std::vector<Stats> out;
      for (int b = first; b < last; ++b) {
        out.push_back(SimulateBlock(option, model, s, b));
      }
      return out;
    }));
  }
  Stats total;
  for (auto& f : futures) {
    for (const Stats& b : f.get()) total.Merge(b);  // block order
  }
  return ToEstimate(total, option, model);
}

Estimate PriceWithMutex(const ContinuousTimeOption& option,
                        const BlackScholesModel& model,
                        const ParallelSettings& s) {
  Validate(s);
  Stats total;
  std::mutex mutex;
  std::atomic<int> next{0};
  {
    std::vector<std::jthread> workers;
    for (int t = 0; t < std::min(s.n_threads, s.n_blocks); ++t) {
      workers.emplace_back([&] {
        for (int b = next.fetch_add(1); b < s.n_blocks; b = next.fetch_add(1)) {
          const Stats block = SimulateBlock(option, model, s, b);
          const std::scoped_lock lock(mutex);
          total.Merge(block);
        }
      });
    }
  }
  return ToEstimate(total, option, model);
}

}  // namespace cpp4q::armstrong::ch20
