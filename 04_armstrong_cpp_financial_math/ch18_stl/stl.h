// Chapter 18: The Standard Template Library.
//
// Containers, iterators and algorithms applied to risk and trade data:
// historical VaR and expected shortfall with nth_element (O(n), no full
// sort), histograms in a std::map, aggregating trades by ticker, a
// rolling mean over a std::deque window, the n largest positions with
// partial_sort, and a calendar of unique business dates in a std::set.

#ifndef CPP4Q_ARMSTRONG_CH18_STL_STL_H_
#define CPP4Q_ARMSTRONG_CH18_STL_STL_H_

#include <cstddef>
#include <map>
#include <set>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace cpp4q::armstrong::ch18 {

// Loss quantile at `confidence` (e.g. 0.99) of the empirical distribution
// of `losses` (positive = loss), the smallest loss L with at least
// confidence * n observations <= L. Throws on empty input or confidence
// outside (0, 1).
double HistoricalVaR(std::span<const double> losses, double confidence);
// Mean of the losses at or above HistoricalVaR (the tail average).
double HistoricalExpectedShortfall(std::span<const double> losses,
                                   double confidence);

// Counts per bin: key k holds values in [k w, (k + 1) w).
std::map<long long, std::size_t> Histogram(std::span<const double> values,
                                           double bin_width);

struct Trade {
  std::string ticker;
  double quantity = 0.0;  // signed: negative is a sale
  double price = 0.0;
};

struct Position {
  double quantity = 0.0;
  double cost = 0.0;  // sum of quantity * price
};

// Net position per ticker, sorted by ticker; positions that net to zero
// quantity are dropped.
std::map<std::string, Position> AggregateTrades(std::span<const Trade> trades);

// The n tickers with the largest |quantity * mark|, largest first.
std::vector<std::pair<std::string, double>> LargestExposures(
    const std::map<std::string, Position>& positions,
    const std::map<std::string, double>& marks, std::size_t n);

// Mean of each window of `window` consecutive values (size - window + 1
// results), keeping a running sum over a deque.
std::vector<double> RollingMean(std::span<const double> values,
                                std::size_t window);

// Sorted unique dates (ISO strings sort chronologically).
std::set<std::string> UniqueDates(std::span<const std::string> dates);

}  // namespace cpp4q::armstrong::ch18

#endif  // CPP4Q_ARMSTRONG_CH18_STL_STL_H_
