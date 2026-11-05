// Chapter 5: Strategies, decoration, and statistics.
//
// The Monte Carlo loop should not decide which statistics to keep. It hands
// every discounted pay-off to a StatisticsMc *strategy*, which accumulates
// whatever it wants and reports a table of results at the end. Decorators
// are statistics gatherers that wrap other gatherers: ConvergenceTable
// snapshots its inner gatherer at 1, 2, 4, 8, ... paths, without the inner
// gatherer or the Monte Carlo loop knowing.

#ifndef CPP4Q_JOSHI_CH05_STRATEGIES_STATISTICS_STATISTICS_H_
#define CPP4Q_JOSHI_CH05_STRATEGIES_STATISTICS_STATISTICS_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "ch01_simple_monte_carlo/simple_mc.h"
#include "ch04_virtual_constructor/parameters.h"
#include "ch04_virtual_constructor/vanilla.h"
#include "ch05_strategies_statistics/wrapper.h"

namespace cpp4q::joshi::ch05 {

using ResultTable = std::vector<std::vector<double>>;

class StatisticsMc {
 public:
  virtual ~StatisticsMc() = default;
  virtual void DumpOneResult(double result) = 0;
  // Several results per path at once (e.g. the cash flows of one path of a
  // path-dependent product summed and discounted). Default: one at a time.
  virtual void DumpResults(const double* results, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) DumpOneResult(results[i]);
  }
  virtual ResultTable GetResultsSoFar() const = 0;
  virtual std::unique_ptr<StatisticsMc> Clone() const = 0;

 protected:
  StatisticsMc() = default;
  StatisticsMc(const StatisticsMc&) = default;
  StatisticsMc& operator=(const StatisticsMc&) = default;
};

// One row: {mean}.
class StatisticsMean final : public StatisticsMc {
 public:
  void DumpOneResult(double result) override;
  ResultTable GetResultsSoFar() const override;
  std::unique_ptr<StatisticsMc> Clone() const override;

 private:
  double running_sum_ = 0.0;
  std::uint64_t paths_done_ = 0;
};

// One row: {mean, standard error, variance, skewness, excess kurtosis}.
//
// Accumulates power sums of (x - K), where the shift K is the first result.
// Shifting removes the cancellation that makes raw power sums useless when
// the mean is large compared with the spread (the test adds 1e9 to every
// result); what is left is cancellation of order |mean - K| / sd, which is
// harmless unless the first result is a far outlier. Four multiply-adds per
// result and no division, against ~60 instructions for the Welford /
// Terriberry update of the central moments, which this replaced (see
// profiling/REPORT.md).
class StatisticsMoments final : public StatisticsMc {
 public:
  void DumpOneResult(double result) override;
  ResultTable GetResultsSoFar() const override;
  std::unique_ptr<StatisticsMc> Clone() const override;

 private:
  std::uint64_t n_ = 0;
  double shift_ = 0.0;
  double s1_ = 0.0;  // sum of (x - shift)^k, k = 1..4
  double s2_ = 0.0;
  double s3_ = 0.0;
  double s4_ = 0.0;
};

// Keeps every result; one row: the requested lower-tail quantiles (e.g. a
// P&L gatherer's value at risk). Quantile q is the order statistic of rank
// ceil(q n) (the empirical inverse cdf).
class StatisticsQuantiles final : public StatisticsMc {
 public:
  explicit StatisticsQuantiles(std::vector<double> levels);
  void DumpOneResult(double result) override;
  ResultTable GetResultsSoFar() const override;
  std::unique_ptr<StatisticsMc> Clone() const override;

 private:
  std::vector<double> levels_;
  std::vector<double> results_;
};

// Decorator. Each row is the inner gatherer's row with the number of paths
// appended, taken after 1, 2, 4, ... paths and after the last path.
class ConvergenceTable final : public StatisticsMc {
 public:
  explicit ConvergenceTable(Wrapper<StatisticsMc> inner);
  void DumpOneResult(double result) override;
  ResultTable GetResultsSoFar() const override;
  std::unique_ptr<StatisticsMc> Clone() const override;

 private:
  void Snapshot(ResultTable& into) const;

  Wrapper<StatisticsMc> inner_;
  ResultTable results_so_far_;
  std::uint64_t stopping_point_ = 1;
  std::uint64_t paths_done_ = 0;
};

// Chapter 4's routine with the statistics delegated to `gatherer`, which
// receives each discounted pay-off.
void SimpleMonteCarlo5(const ch04::VanillaOption& option, double spot,
                       const ch04::Parameters& vol, const ch04::Parameters& r,
                       std::uint64_t num_paths, StatisticsMc& gatherer,
                       Rng& rng);

}  // namespace cpp4q::joshi::ch05

#endif  // CPP4Q_JOSHI_CH05_STRATEGIES_STATISTICS_STATISTICS_H_
