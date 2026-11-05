#include "ch18_stl/stl.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>
#include <stdexcept>

namespace cpp4q::armstrong::ch18 {
namespace {

std::size_t VarIndex(std::size_t n, double confidence) {
  if (n == 0) throw std::invalid_argument("no losses");
  if (!(confidence > 0 && confidence < 1)) {
    throw std::invalid_argument("confidence must be in (0, 1)");
  }
  // Smallest k (1-based) with k >= confidence * n, as a 0-based index.
  const auto k = static_cast<std::size_t>(
      std::ceil(confidence * static_cast<double>(n) - 1e-9));
  return std::clamp<std::size_t>(k, 1, n) - 1;
}

}  // namespace

double HistoricalVaR(std::span<const double> losses, double confidence) {
  const std::size_t k = VarIndex(losses.size(), confidence);
  std::vector<double> v(losses.begin(), losses.end());
  std::nth_element(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(k),
                   v.end());
  return v[k];
}

double HistoricalExpectedShortfall(std::span<const double> losses,
                                   double confidence) {
  const std::size_t k = VarIndex(losses.size(), confidence);
  std::vector<double> v(losses.begin(), losses.end());
  const auto kth = v.begin() + static_cast<std::ptrdiff_t>(k);
  std::nth_element(v.begin(), kth, v.end());
  // After nth_element everything from kth on is >= *kth.
  return std::accumulate(kth, v.end(), 0.0) /
         static_cast<double>(v.end() - kth);
}

std::map<long long, std::size_t> Histogram(std::span<const double> values,
                                           double bin_width) {
  if (!(bin_width > 0)) throw std::invalid_argument("bin_width must be > 0");
  std::map<long long, std::size_t> bins;
  for (double x : values) {
    ++bins[static_cast<long long>(std::floor(x / bin_width))];
  }
  return bins;
}

std::map<std::string, Position> AggregateTrades(std::span<const Trade> trades) {
  std::map<std::string, Position> book;
  for (const Trade& t : trades) {
    Position& p = book[t.ticker];
    p.quantity += t.quantity;
    p.cost += t.quantity * t.price;
  }
  std::erase_if(book, [](const auto& kv) { return kv.second.quantity == 0; });
  return book;
}

std::vector<std::pair<std::string, double>> LargestExposures(
    const std::map<std::string, Position>& positions,
    const std::map<std::string, double>& marks, std::size_t n) {
  std::vector<std::pair<std::string, double>> exposures;
  exposures.reserve(positions.size());
  for (const auto& [ticker, pos] : positions) {
    const auto it = marks.find(ticker);
    if (it == marks.end()) {
      throw std::invalid_argument("no mark for " + ticker);
    }
    exposures.emplace_back(ticker, pos.quantity * it->second);
  }
  n = std::min(n, exposures.size());
  const auto by_size = [](const auto& a, const auto& b) {
    return std::abs(a.second) > std::abs(b.second);
  };
  std::partial_sort(exposures.begin(),
                    exposures.begin() + static_cast<std::ptrdiff_t>(n),
                    exposures.end(), by_size);
  exposures.resize(n);
  return exposures;
}

std::vector<double> RollingMean(std::span<const double> values,
                                std::size_t window) {
  if (window == 0) throw std::invalid_argument("window must be >= 1");
  std::vector<double> out;
  if (values.size() < window) return out;
  out.reserve(values.size() - window + 1);
  std::deque<double> buffer;
  double sum = 0.0;
  for (double x : values) {
    buffer.push_back(x);
    sum += x;
    if (buffer.size() > window) {
      sum -= buffer.front();
      buffer.pop_front();
    }
    if (buffer.size() == window) {
      out.push_back(sum / static_cast<double>(window));
    }
  }
  return out;
}

std::set<std::string> UniqueDates(std::span<const std::string> dates) {
  return {dates.begin(), dates.end()};
}

}  // namespace cpp4q::armstrong::ch18
