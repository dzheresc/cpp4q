// Chapter 6: Random Number Generation and Concurrency.
//
// <random> engines and distributions drive GBM Monte Carlo pricers. The
// parallel variants give every task its own engine, seeded through
// std::seed_seq from (seed, task index), so results are reproducible for a
// fixed thread count and no engine is ever shared across threads.

#ifndef CPP4Q_HANSON_CH06_RANDOM_CONCURRENCY_MONTE_CARLO_H_
#define CPP4Q_HANSON_CH06_RANDOM_CONCURRENCY_MONTE_CARLO_H_

#include <cstddef>
#include <cstdint>

namespace cpp4q::hanson::ch06 {

enum class OptionType { kCall, kPut };

struct GbmParams {
  double spot = 100.0;
  double rate = 0.0;
  double dividend_yield = 0.0;
  double volatility = 0.2;
  double expiry = 1.0;
};

struct McConfig {
  std::size_t num_paths = 100'000;
  std::uint64_t seed = 42;
  bool antithetic = true;
};

struct McResult {
  double price = 0.0;
  double std_error = 0.0;
  std::size_t num_samples = 0;
};

// Running first/second moments that can be merged across threads.
class SampleStats {
 public:
  void Add(double x) {
    sum_ += x;
    sum_sq_ += x * x;
    ++count_;
  }
  void Merge(const SampleStats& other) {
    sum_ += other.sum_;
    sum_sq_ += other.sum_sq_;
    count_ += other.count_;
  }
  [[nodiscard]] std::size_t count() const { return count_; }
  [[nodiscard]] double Mean() const;
  [[nodiscard]] double StdError() const;

 private:
  double sum_ = 0.0;
  double sum_sq_ = 0.0;
  std::size_t count_ = 0;
};

// European option on terminal GBM spot (exact one-step simulation).
McResult PriceEuropeanMc(OptionType type, double strike, const GbmParams& gbm,
                         const McConfig& config);

// Arithmetic-average Asian option, `num_steps` equally spaced fixings.
McResult PriceAsianMc(OptionType type, double strike, const GbmParams& gbm,
                      std::size_t num_steps, const McConfig& config);

// Splits config.num_paths across `num_threads` std::async tasks.
McResult PriceEuropeanMcParallel(OptionType type, double strike,
                                 const GbmParams& gbm, const McConfig& config,
                                 unsigned num_threads);

// Same split using std::jthread workers.
McResult PriceAsianMcParallel(OptionType type, double strike,
                              const GbmParams& gbm, std::size_t num_steps,
                              const McConfig& config, unsigned num_threads);

}  // namespace cpp4q::hanson::ch06

#endif  // CPP4Q_HANSON_CH06_RANDOM_CONCURRENCY_MONTE_CARLO_H_
