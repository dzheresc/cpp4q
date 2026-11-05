#include "ch06_random_concurrency/monte_carlo.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

namespace cpp4q::hanson::ch06 {
namespace {

double Payoff(OptionType type, double s, double k) {
  return type == OptionType::kCall ? std::max(s - k, 0.0)
                                   : std::max(k - s, 0.0);
}

std::mt19937_64 MakeEngine(std::uint64_t seed, std::uint64_t stream) {
  std::seed_seq seq{static_cast<std::uint32_t>(seed),
                    static_cast<std::uint32_t>(seed >> 32),
                    static_cast<std::uint32_t>(stream),
                    static_cast<std::uint32_t>(stream >> 32)};
  return std::mt19937_64(seq);
}

McResult ToResult(const SampleStats& stats, double discount) {
  return {discount * stats.Mean(), discount * stats.StdError(), stats.count()};
}

// Simulates `num_paths` terminal-spot samples into `stats`.
void SimulateEuropean(OptionType type, double strike, const GbmParams& gbm,
                      std::size_t num_paths, bool antithetic,
                      std::mt19937_64& engine, SampleStats& stats) {
  std::normal_distribution<double> normal(0.0, 1.0);
  const double t = gbm.expiry;
  const double drift =
      (gbm.rate - gbm.dividend_yield - 0.5 * gbm.volatility * gbm.volatility) *
      t;
  const double vol_sqrt_t = gbm.volatility * std::sqrt(t);
  // Antithetic pair: S e^(m+vz) and S e^(m-vz) multiply to S^2 e^(2m), so the
  // mirror path costs a division instead of a second std::exp (exp was the
  // top self-cost symbol under callgrind).
  const double spot_sq_growth = gbm.spot * gbm.spot * std::exp(2.0 * drift);
  for (std::size_t i = 0; i < num_paths; ++i) {
    const double z = normal(engine);
    const double s_up = gbm.spot * std::exp(drift + vol_sqrt_t * z);
    const double up = Payoff(type, s_up, strike);
    if (antithetic) {
      const double down = Payoff(type, spot_sq_growth / s_up, strike);
      stats.Add(0.5 * (up + down));  // one sample per antithetic pair
    } else {
      stats.Add(up);
    }
  }
}

void SimulateAsian(OptionType type, double strike, const GbmParams& gbm,
                   std::size_t num_steps, std::size_t num_paths,
                   bool antithetic, std::mt19937_64& engine,
                   SampleStats& stats) {
  std::normal_distribution<double> normal(0.0, 1.0);
  const double dt = gbm.expiry / static_cast<double>(num_steps);
  const double drift =
      (gbm.rate - gbm.dividend_yield - 0.5 * gbm.volatility * gbm.volatility) *
      dt;
  const double vol_sqrt_dt = gbm.volatility * std::sqrt(dt);
  const double inv_steps = 1.0 / static_cast<double>(num_steps);
  const double growth_sq = std::exp(2.0 * drift);  // see SimulateEuropean

  for (std::size_t p = 0; p < num_paths; ++p) {
    double s_up = gbm.spot;
    double s_down = gbm.spot;
    double sum_up = 0.0;
    double sum_down = 0.0;
    for (std::size_t k = 0; k < num_steps; ++k) {
      const double z = normal(engine);
      const double g = std::exp(drift + vol_sqrt_dt * z);
      s_up *= g;
      sum_up += s_up;
      if (antithetic) {
        s_down *= growth_sq / g;
        sum_down += s_down;
      }
    }
    const double up = Payoff(type, sum_up * inv_steps, strike);
    if (antithetic) {
      stats.Add(0.5 * (up + Payoff(type, sum_down * inv_steps, strike)));
    } else {
      stats.Add(up);
    }
  }
}

void Validate(const GbmParams& gbm, double strike, std::size_t num_paths) {
  if (!(gbm.spot > 0.0) || !(strike > 0.0) || !(gbm.volatility >= 0.0) ||
      !(gbm.expiry >= 0.0) || num_paths == 0) {
    throw std::invalid_argument("Monte Carlo: invalid inputs");
  }
}

// Paths for chunk `i` of `n` (the remainder goes to the first chunks).
std::size_t ChunkSize(std::size_t total, unsigned n, unsigned i) {
  return total / n + (i < total % n ? 1 : 0);
}

}  // namespace

double SampleStats::Mean() const {
  return count_ == 0 ? 0.0 : sum_ / static_cast<double>(count_);
}

double SampleStats::StdError() const {
  if (count_ < 2) return 0.0;
  const auto n = static_cast<double>(count_);
  const double mean = sum_ / n;
  const double var = std::max(0.0, (sum_sq_ - n * mean * mean) / (n - 1.0));
  return std::sqrt(var / n);
}

McResult PriceEuropeanMc(OptionType type, double strike, const GbmParams& gbm,
                         const McConfig& config) {
  Validate(gbm, strike, config.num_paths);
  auto engine = MakeEngine(config.seed, 0);
  SampleStats stats;
  SimulateEuropean(type, strike, gbm, config.num_paths, config.antithetic,
                   engine, stats);
  return ToResult(stats, std::exp(-gbm.rate * gbm.expiry));
}

McResult PriceAsianMc(OptionType type, double strike, const GbmParams& gbm,
                      std::size_t num_steps, const McConfig& config) {
  Validate(gbm, strike, config.num_paths);
  if (num_steps == 0) throw std::invalid_argument("Asian: num_steps == 0");
  auto engine = MakeEngine(config.seed, 0);
  SampleStats stats;
  SimulateAsian(type, strike, gbm, num_steps, config.num_paths,
                config.antithetic, engine, stats);
  return ToResult(stats, std::exp(-gbm.rate * gbm.expiry));
}

McResult PriceEuropeanMcParallel(OptionType type, double strike,
                                 const GbmParams& gbm, const McConfig& config,
                                 unsigned num_threads) {
  Validate(gbm, strike, config.num_paths);
  num_threads = std::max(1u, num_threads);
  std::vector<std::future<SampleStats>> futures;
  futures.reserve(num_threads);
  for (unsigned i = 0; i < num_threads; ++i) {
    futures.push_back(std::async(std::launch::async, [=] {
      auto engine = MakeEngine(config.seed, i);
      SampleStats local;
      SimulateEuropean(type, strike, gbm,
                       ChunkSize(config.num_paths, num_threads, i),
                       config.antithetic, engine, local);
      return local;
    }));
  }
  SampleStats total;
  for (auto& f : futures) total.Merge(f.get());  // rethrows task exceptions
  return ToResult(total, std::exp(-gbm.rate * gbm.expiry));
}

McResult PriceAsianMcParallel(OptionType type, double strike,
                              const GbmParams& gbm, std::size_t num_steps,
                              const McConfig& config, unsigned num_threads) {
  Validate(gbm, strike, config.num_paths);
  if (num_steps == 0) throw std::invalid_argument("Asian: num_steps == 0");
  num_threads = std::max(1u, num_threads);
  std::vector<SampleStats> partial(num_threads);
  {
    std::vector<std::jthread> workers;
    workers.reserve(num_threads);
    for (unsigned i = 0; i < num_threads; ++i) {
      workers.emplace_back([&, i] {
        // Accumulate in a thread-local object and publish once: adjacent
        // SampleStats in `partial` share cache lines (false sharing).
        auto engine = MakeEngine(config.seed, i);
        SampleStats local;
        SimulateAsian(type, strike, gbm, num_steps,
                      ChunkSize(config.num_paths, num_threads, i),
                      config.antithetic, engine, local);
        partial[i] = local;
      });
    }
  }  // jthreads join here
  SampleStats total;
  for (const auto& s : partial) total.Merge(s);
  return ToResult(total, std::exp(-gbm.rate * gbm.expiry));
}

}  // namespace cpp4q::hanson::ch06
