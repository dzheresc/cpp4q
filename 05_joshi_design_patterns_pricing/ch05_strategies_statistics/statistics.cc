#include "ch05_strategies_statistics/statistics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace cpp4q::joshi::ch05 {

void StatisticsMean::DumpOneResult(double result) {
  ++paths_done_;
  running_sum_ += result;
}

ResultTable StatisticsMean::GetResultsSoFar() const {
  return {{running_sum_ / static_cast<double>(paths_done_)}};
}

std::unique_ptr<StatisticsMc> StatisticsMean::Clone() const {
  return std::make_unique<StatisticsMean>(*this);
}

void StatisticsMoments::DumpOneResult(double x) {
  if (n_ == 0) shift_ = x;
  ++n_;
  const double y = x - shift_;
  const double y2 = y * y;
  s1_ += y;
  s2_ += y2;
  s3_ += y2 * y;
  s4_ += y2 * y2;
}

ResultTable StatisticsMoments::GetResultsSoFar() const {
  const double n = static_cast<double>(n_);
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double mu = s1_ / n;  // mean - shift
  // Central sums from the shifted power sums (binomial expansion).
  const double m2 = std::max(s2_ - mu * s1_, 0.0);
  const double m3 = s3_ - 3.0 * mu * s2_ + 2.0 * mu * mu * s1_;
  const double m4 =
      s4_ - 4.0 * mu * s3_ + 6.0 * mu * mu * s2_ - 3.0 * mu * mu * mu * s1_;
  const double variance = n_ > 1 ? m2 / (n - 1) : nan;
  const double std_error = std::sqrt(variance / n);
  const double skew = m2 > 0 ? std::sqrt(n) * m3 / std::pow(m2, 1.5) : nan;
  const double kurt = m2 > 0 ? n * m4 / (m2 * m2) - 3.0 : nan;
  return {{shift_ + mu, std_error, variance, skew, kurt}};
}

std::unique_ptr<StatisticsMc> StatisticsMoments::Clone() const {
  return std::make_unique<StatisticsMoments>(*this);
}

StatisticsQuantiles::StatisticsQuantiles(std::vector<double> levels)
    : levels_(std::move(levels)) {
  for (const double q : levels_) {
    if (!(q > 0.0 && q <= 1.0)) {
      throw std::invalid_argument("StatisticsQuantiles: level not in (0, 1]");
    }
  }
}

void StatisticsQuantiles::DumpOneResult(double result) {
  results_.push_back(result);
}

ResultTable StatisticsQuantiles::GetResultsSoFar() const {
  if (results_.empty()) throw std::logic_error("StatisticsQuantiles: empty");
  std::vector<double> work = results_;
  std::vector<double> row;
  row.reserve(levels_.size());
  for (const double q : levels_) {
    const auto rank = static_cast<std::size_t>(
        std::ceil(q * static_cast<double>(work.size())));
    const auto nth = work.begin() + static_cast<std::ptrdiff_t>(
                                        std::max<std::size_t>(rank, 1) - 1);
    std::nth_element(work.begin(), nth, work.end());
    row.push_back(*nth);
  }
  return {row};
}

std::unique_ptr<StatisticsMc> StatisticsQuantiles::Clone() const {
  return std::make_unique<StatisticsQuantiles>(*this);
}

ConvergenceTable::ConvergenceTable(Wrapper<StatisticsMc> inner)
    : inner_(std::move(inner)) {
  if (!inner_) throw std::invalid_argument("ConvergenceTable: no inner");
}

void ConvergenceTable::Snapshot(ResultTable& into) const {
  for (std::vector<double>& row : inner_->GetResultsSoFar()) {
    row.push_back(static_cast<double>(paths_done_));
    into.push_back(std::move(row));
  }
}

void ConvergenceTable::DumpOneResult(double result) {
  inner_->DumpOneResult(result);
  ++paths_done_;
  if (paths_done_ == stopping_point_) {
    stopping_point_ *= 2;
    Snapshot(results_so_far_);
  }
}

ResultTable ConvergenceTable::GetResultsSoFar() const {
  ResultTable table = results_so_far_;
  if (paths_done_ * 2 != stopping_point_) Snapshot(table);
  return table;
}

std::unique_ptr<StatisticsMc> ConvergenceTable::Clone() const {
  return std::make_unique<ConvergenceTable>(*this);
}

void SimpleMonteCarlo5(const ch04::VanillaOption& option, double spot,
                       const ch04::Parameters& vol, const ch04::Parameters& r,
                       std::uint64_t num_paths, StatisticsMc& gatherer,
                       Rng& rng) {
  const double expiry = option.expiry();
  const double variance = vol.IntegralSquare(0, expiry);
  const double root_variance = std::sqrt(variance);
  const double int_r = r.Integral(0, expiry);
  const double moved_spot = spot * std::exp(int_r - 0.5 * variance);
  const double df = std::exp(-int_r);
  for (std::uint64_t i = 0; i < num_paths; ++i) {
    const double this_spot =
        moved_spot * std::exp(root_variance * ch01::GetOneGaussianByPolar(rng));
    gatherer.DumpOneResult(df * option.OptionPayOff(this_spot));
  }
}

}  // namespace cpp4q::joshi::ch05
