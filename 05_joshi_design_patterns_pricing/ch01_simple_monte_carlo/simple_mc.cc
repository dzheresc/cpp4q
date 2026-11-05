#include "ch01_simple_monte_carlo/simple_mc.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace cpp4q::joshi::ch01 {

double GetOneGaussianBySummation(Rng& rng) {
  double sum = 0.0;
  for (int i = 0; i < 12; ++i) sum += UniformOpen(rng);
  return sum - 6.0;
}

double GetOneGaussianByBoxMuller(Rng& rng) {
  const double u1 = UniformOpen(rng);
  const double u2 = UniformOpen(rng);
  return std::sqrt(-2.0 * std::log(u1)) *
         std::cos(2.0 * std::numbers::pi * u2);
}

double GetOneGaussianByPolar(Rng& rng) {
  double x;
  double size_squared;
  do {
    x = 2.0 * UniformOpen(rng) - 1.0;
    const double y = 2.0 * UniformOpen(rng) - 1.0;
    size_squared = x * x + y * y;
  } while (size_squared >= 1.0);
  return x * std::sqrt(-2.0 * std::log(size_squared) / size_squared);
}

McResult SimpleMonteCarlo1(double expiry, double strike, double spot,
                           double vol, double r, std::uint64_t num_paths,
                           Rng& rng) {
  const double variance = vol * vol * expiry;
  const double root_variance = std::sqrt(variance);
  const double moved_spot = spot * std::exp(r * expiry - 0.5 * variance);
  double sum = 0.0;
  double sum_sq = 0.0;
  for (std::uint64_t i = 0; i < num_paths; ++i) {
    const double this_spot =
        moved_spot * std::exp(root_variance * GetOneGaussianByPolar(rng));
    const double payoff = std::max(this_spot - strike, 0.0);
    sum += payoff;
    sum_sq += payoff * payoff;
  }
  const double n = static_cast<double>(num_paths);
  const double mean = sum / n;
  const double var = std::max(sum_sq / n - mean * mean, 0.0);
  const double df = std::exp(-r * expiry);
  return {df * mean, df * std::sqrt(var / n)};
}

}  // namespace cpp4q::joshi::ch01
