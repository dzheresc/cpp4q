#include "ch03_inheritance/payoff2.h"

#include <algorithm>
#include <cmath>

namespace cpp4q::joshi::ch03 {

double PayOffCall::operator()(double spot) const {
  return std::max(spot - strike_, 0.0);
}

double PayOffPut::operator()(double spot) const {
  return std::max(strike_ - spot, 0.0);
}

double PayOffDoubleDigital::operator()(double spot) const {
  return spot >= lower_ && spot <= upper_ ? 1.0 : 0.0;
}

McResult SimpleMonteCarlo3(const PayOff& payoff, double expiry, double spot,
                           double vol, double r, std::uint64_t num_paths,
                           Rng& rng) {
  const double variance = vol * vol * expiry;
  const double root_variance = std::sqrt(variance);
  const double moved_spot = spot * std::exp(r * expiry - 0.5 * variance);
  double sum = 0.0;
  double sum_sq = 0.0;
  for (std::uint64_t i = 0; i < num_paths; ++i) {
    const double value = payoff(
        moved_spot * std::exp(root_variance * ch01::GetOneGaussianByPolar(rng)));
    sum += value;
    sum_sq += value * value;
  }
  const double n = static_cast<double>(num_paths);
  const double mean = sum / n;
  const double var = std::max(sum_sq / n - mean * mean, 0.0);
  const double df = std::exp(-r * expiry);
  return {df * mean, df * std::sqrt(var / n)};
}

}  // namespace cpp4q::joshi::ch03
