#include "ch02_encapsulation/payoff1.h"

#include <algorithm>
#include <cmath>

namespace cpp4q::joshi::ch02 {

double PayOff::operator()(double spot) const {
  switch (type_) {
    case OptionType::kCall:
      return std::max(spot - strike_, 0.0);
    case OptionType::kPut:
      return std::max(strike_ - spot, 0.0);
  }
  return 0.0;  // unreachable for valid enumerators
}

McResult SimpleMonteCarlo2(const PayOff& payoff, double expiry, double spot,
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

}  // namespace cpp4q::joshi::ch02
