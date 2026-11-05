#include "ch04_virtual_constructor/vanilla.h"

#include <algorithm>
#include <cmath>

namespace cpp4q::joshi::ch04 {

McResult SimpleMonteCarlo4(const VanillaOption& option, double spot,
                           const Parameters& vol, const Parameters& r,
                           std::uint64_t num_paths, Rng& rng) {
  const double expiry = option.expiry();
  const double variance = vol.IntegralSquare(0, expiry);
  const double root_variance = std::sqrt(variance);
  const double int_r = r.Integral(0, expiry);
  const double moved_spot = spot * std::exp(int_r - 0.5 * variance);
  double sum = 0.0;
  double sum_sq = 0.0;
  for (std::uint64_t i = 0; i < num_paths; ++i) {
    const double value = option.OptionPayOff(
        moved_spot * std::exp(root_variance * ch01::GetOneGaussianByPolar(rng)));
    sum += value;
    sum_sq += value * value;
  }
  const double n = static_cast<double>(num_paths);
  const double mean = sum / n;
  const double var = std::max(sum_sq / n - mean * mean, 0.0);
  const double df = std::exp(-int_r);
  return {df * mean, df * std::sqrt(var / n)};
}

}  // namespace cpp4q::joshi::ch04
