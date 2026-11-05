// Chapter 5: The STL Part II - Algorithms and Ranges.
//
// Return and risk statistics written with <algorithm>, <numeric> and
// std::ranges / views instead of hand-rolled loops.

#ifndef CPP4Q_HANSON_CH05_ALGORITHMS_RANGES_RETURN_STATS_H_
#define CPP4Q_HANSON_CH05_ALGORITHMS_RANGES_RETURN_STATS_H_

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace cpp4q::hanson::ch05 {

// r_i = p_i / p_{i-1} - 1. Output has prices.size() - 1 elements.
std::vector<double> SimpleReturns(std::span<const double> prices);
// r_i = log(p_i / p_{i-1}).
std::vector<double> LogReturns(std::span<const double> prices);

double Mean(std::span<const double> xs);
// Sample variance (n - 1), two-pass. 0 for fewer than two points.
double SampleVariance(std::span<const double> xs);
double SampleStdDev(std::span<const double> xs);

// Annualised Sharpe ratio of periodic returns.
double SharpeRatio(std::span<const double> returns, double periodic_rf,
                   double periods_per_year);

// Largest peak-to-trough decline as a positive fraction of the peak.
double MaxDrawdown(std::span<const double> prices);

// Simple moving average with window `n` (output size prices.size()-n+1).
// O(size) sliding sum.
std::vector<double> MovingAverage(std::span<const double> prices,
                                  std::size_t n);
// Same result as a views::iota | views::transform pipeline that re-reduces
// every window: O(size * n). Kept as the readable reference and as the
// "before" in bench/ (see profiling/REPORT.md).
std::vector<double> MovingAverageRanges(std::span<const double> prices,
                                        std::size_t n);

// Historical VaR at `confidence` (e.g. 0.99) as a positive loss fraction,
// using the empirical quantile: the (floor((1-c) n))-th smallest return,
// found with nth_element (O(n) average).
double HistoricalVar(std::span<const double> returns, double confidence);

// Portfolio return = sum w_i r_i (std::inner_product).
double PortfolioReturn(std::span<const double> weights,
                       std::span<const double> asset_returns);

struct Ticker {
  std::string symbol;
  double total_return;
};
// The `k` best performers, best first (partial_sort_copy, O(n log k)).
std::vector<Ticker> TopPerformers(std::span<const Ticker> universe,
                                  std::size_t k);

// Sum of positive returns, via a views::filter | views::transform pipeline.
double SumOfGains(std::span<const double> returns);

}  // namespace cpp4q::hanson::ch05

#endif  // CPP4Q_HANSON_CH05_ALGORITHMS_RANGES_RETURN_STATS_H_
