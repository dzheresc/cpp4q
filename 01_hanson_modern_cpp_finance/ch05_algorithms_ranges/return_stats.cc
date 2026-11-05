#include "ch05_algorithms_ranges/return_stats.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <stdexcept>

namespace cpp4q::hanson::ch05 {
namespace {

template <typename BinaryOp>
std::vector<double> PairwiseTransform(std::span<const double> xs, BinaryOp op) {
  if (xs.size() < 2) return {};
  std::vector<double> out(xs.size() - 1);
  // Two-input std::transform over [x_1..x_n] and [x_0..x_{n-1}].
  std::transform(xs.begin() + 1, xs.end(), xs.begin(), out.begin(), op);
  return out;
}

}  // namespace

std::vector<double> SimpleReturns(std::span<const double> prices) {
  return PairwiseTransform(
      prices, [](double now, double prev) { return now / prev - 1.0; });
}

std::vector<double> LogReturns(std::span<const double> prices) {
  return PairwiseTransform(
      prices, [](double now, double prev) { return std::log(now / prev); });
}

double Mean(std::span<const double> xs) {
  if (xs.empty()) return 0.0;
  return std::reduce(xs.begin(), xs.end(), 0.0) /
         static_cast<double>(xs.size());
}

double SampleVariance(std::span<const double> xs) {
  if (xs.size() < 2) return 0.0;
  const double mu = Mean(xs);
  const double ss =
      std::transform_reduce(xs.begin(), xs.end(), 0.0, std::plus<>(),
                            [mu](double x) { return (x - mu) * (x - mu); });
  return ss / static_cast<double>(xs.size() - 1);
}

double SampleStdDev(std::span<const double> xs) {
  return std::sqrt(SampleVariance(xs));
}

double SharpeRatio(std::span<const double> returns, double periodic_rf,
                   double periods_per_year) {
  const double sd = SampleStdDev(returns);
  if (sd == 0.0) return 0.0;
  return (Mean(returns) - periodic_rf) / sd * std::sqrt(periods_per_year);
}

double MaxDrawdown(std::span<const double> prices) {
  // One pass, no allocation: track the running peak and the worst ratio.
  // (A first version materialised the peaks with inclusive_scan and then
  // transform_reduce'd; massif showed the extra n-sized buffer and it ran
  // ~3x slower than the other O(n) statistics.)
  double peak = -std::numeric_limits<double>::infinity();
  double worst = 0.0;
  for (const double p : prices) {
    peak = std::max(peak, p);
    worst = std::max(worst, 1.0 - p / peak);
  }
  return worst;
}

std::vector<double> MovingAverage(std::span<const double> prices,
                                  std::size_t n) {
  if (n == 0) throw std::invalid_argument("MovingAverage: n must be > 0");
  if (prices.size() < n) return {};
  // Sliding sum: O(size) instead of O(size * n). Callgrind put the
  // reduce-every-window version at 65% of the statistics workload. The sum
  // is recomputed exactly every n outputs so rounding error cannot build up.
  const auto inv_n = 1.0 / static_cast<double>(n);
  std::vector<double> out(prices.size() - n + 1);
  double sum = std::reduce(
      prices.begin(), prices.begin() + static_cast<std::ptrdiff_t>(n), 0.0);
  out[0] = sum * inv_n;
  for (std::size_t i = 1; i < out.size(); ++i) {
    if (i % n == 0) {
      const auto w = prices.subspan(i, n);
      sum = std::reduce(w.begin(), w.end(), 0.0);
    } else {
      sum += prices[i + n - 1] - prices[i - 1];
    }
    out[i] = sum * inv_n;
  }
  return out;
}

std::vector<double> MovingAverageRanges(std::span<const double> prices,
                                        std::size_t n) {
  if (n == 0) throw std::invalid_argument("MovingAverage: n must be > 0");
  if (prices.size() < n) return {};
  // views::slide is C++23; express the windows as an iota of start indices.
  auto windows =
      std::views::iota(std::size_t{0}, prices.size() - n + 1) |
      std::views::transform([prices, n](std::size_t i) {
        const auto w = prices.subspan(i, n);
        return std::reduce(w.begin(), w.end(), 0.0) / static_cast<double>(n);
      });
  // iota<size_t> iterators are C++20-only iterators (128-bit difference
  // type in libstdc++), so copy with ranges::copy rather than the legacy
  // vector(first, last) constructor.
  std::vector<double> out;
  out.reserve(prices.size() - n + 1);
  std::ranges::copy(windows, std::back_inserter(out));
  return out;
}

double HistoricalVar(std::span<const double> returns, double confidence) {
  if (returns.empty()) return 0.0;
  if (!(confidence > 0.0 && confidence < 1.0)) {
    throw std::invalid_argument("HistoricalVar: confidence in (0,1)");
  }
  // Tried a bounded heap (partial_sort_copy into k+1 slots, O(k) memory):
  // 1.6x slower at n = 262k and no change to the massif peak, so the
  // full copy + nth_element (O(n) average) stays.
  std::vector<double> sorted(returns.begin(), returns.end());
  const auto k = static_cast<std::size_t>(
      std::floor((1.0 - confidence) * static_cast<double>(sorted.size())));
  const auto nth = sorted.begin() + static_cast<std::ptrdiff_t>(k);
  std::ranges::nth_element(sorted, nth);
  return std::max(0.0, -*nth);
}

double PortfolioReturn(std::span<const double> weights,
                       std::span<const double> asset_returns) {
  if (weights.size() != asset_returns.size()) {
    throw std::invalid_argument("PortfolioReturn: size mismatch");
  }
  return std::inner_product(weights.begin(), weights.end(),
                            asset_returns.begin(), 0.0);
}

std::vector<Ticker> TopPerformers(std::span<const Ticker> universe,
                                  std::size_t k) {
  std::vector<Ticker> top(std::min(k, universe.size()));
  std::ranges::partial_sort_copy(universe, top, std::ranges::greater(),
                                 &Ticker::total_return, &Ticker::total_return);
  return top;
}

double SumOfGains(std::span<const double> returns) {
  auto gains = returns | std::views::filter([](double r) { return r > 0.0; });
  return std::accumulate(gains.begin(), gains.end(), 0.0);
}

}  // namespace cpp4q::hanson::ch05
