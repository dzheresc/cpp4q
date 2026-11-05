#include "ch05_multiple_files/vectors.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cpp4q::armstrong::ch05 {
namespace {

void RequireNonEmpty(std::span<const double> v) {
  if (v.empty()) throw std::invalid_argument("empty vector");
}

}  // namespace

std::vector<double> Linspace(double a, double b, int n) {
  if (n < 1) throw std::invalid_argument("Linspace needs n >= 1");
  if (n == 1) return {a};
  std::vector<double> out(static_cast<std::size_t>(n));
  const double h = (b - a) / (n - 1);
  for (int i = 0; i < n; ++i) out[static_cast<std::size_t>(i)] = a + i * h;
  out.back() = b;  // exact endpoint regardless of rounding in h
  return out;
}

double Sum(std::span<const double> v) {
  double s = 0.0;
  for (double x : v) s += x;
  return s;
}

double Mean(std::span<const double> v) {
  RequireNonEmpty(v);
  return Sum(v) / static_cast<double>(v.size());
}

double StandardDeviation(std::span<const double> v, bool population) {
  RequireNonEmpty(v);
  if (!population && v.size() < 2) {
    throw std::invalid_argument("sample std needs at least two values");
  }
  double mean = 0.0;
  double m2 = 0.0;
  std::size_t n = 0;
  for (double x : v) {
    ++n;
    const double d = x - mean;
    mean += d / static_cast<double>(n);
    m2 += d * (x - mean);
  }
  return std::sqrt(m2 / static_cast<double>(population ? n : n - 1));
}

double Min(std::span<const double> v) {
  RequireNonEmpty(v);
  return *std::ranges::min_element(v);
}

double Max(std::span<const double> v) {
  RequireNonEmpty(v);
  return *std::ranges::max_element(v);
}

double Prctile(std::span<const double> v, double p) {
  RequireNonEmpty(v);
  if (!(p >= 0 && p <= 100)) throw std::invalid_argument("p not in [0, 100]");
  std::vector<double> sorted(v.begin(), v.end());
  std::ranges::sort(sorted);
  const auto n = static_cast<double>(sorted.size());
  // Position in sorted (0-based) whose percentile is p.
  const double pos = p / 100.0 * n - 0.5;
  if (pos <= 0) return sorted.front();
  if (pos >= n - 1) return sorted.back();
  const auto i = static_cast<std::size_t>(pos);
  const double frac = pos - static_cast<double>(i);
  return sorted[i] + frac * (sorted[i + 1] - sorted[i]);
}

std::vector<double> RandUniform(int n, Rng& rng) {
  if (n < 0) throw std::invalid_argument("n must be >= 0");
  std::uniform_real_distribution<double> u(0.0, 1.0);
  std::vector<double> out(static_cast<std::size_t>(n));
  for (double& x : out) x = u(rng);
  return out;
}

std::vector<double> RandN(int n, Rng& rng) {
  if (n < 0) throw std::invalid_argument("n must be >= 0");
  std::normal_distribution<double> z;
  std::vector<double> out(static_cast<std::size_t>(n));
  for (double& x : out) x = z(rng);
  return out;
}

}  // namespace cpp4q::armstrong::ch05
