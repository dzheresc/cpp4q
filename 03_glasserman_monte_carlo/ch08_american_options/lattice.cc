#include "ch08_american_options/lattice.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace cpp4q::glasserman::ch08 {

double CrrPrice(const BlackScholesInputs& in, OptionType type,
                std::size_t steps, ExerciseStyle style,
                std::size_t exercise_dates) {
  if (steps == 0) throw std::invalid_argument("steps must be > 0");
  std::size_t stride = 1;
  if (style == ExerciseStyle::kBermudan) {
    if (exercise_dates == 0 || steps % exercise_dates != 0) {
      throw std::invalid_argument("steps must be a multiple of the dates");
    }
    stride = steps / exercise_dates;
  }
  const double dt = in.maturity / static_cast<double>(steps);
  const double u = std::exp(in.vol * std::sqrt(dt));
  const double d = 1.0 / u;
  const double growth = std::exp((in.rate - in.dividend) * dt);
  const double p = (growth - d) / (u - d);
  if (!(p > 0.0 && p < 1.0)) throw std::invalid_argument("tree too coarse");
  const double disc = std::exp(-in.rate * dt);
  const double sign = type == OptionType::kCall ? 1.0 : -1.0;
  auto payoff = [&](double s) { return std::max(sign * (s - in.strike), 0.0); };

  // spot[j] = S0 u^(2j - n) at time step n. Stepping back from n to n-1
  // keeps index j and multiplies by u, so no pow() inside the sweep (the
  // first version called pow() at every exercisable node; see
  // profiling/REPORT.md).
  std::vector<double> v(steps + 1);
  std::vector<double> spot(steps + 1);
  spot[0] = in.spot * std::pow(d, static_cast<double>(steps));
  for (std::size_t j = 1; j <= steps; ++j) spot[j] = spot[j - 1] * u * u;
  for (std::size_t j = 0; j <= steps; ++j) v[j] = payoff(spot[j]);
  for (std::size_t n = steps; n-- > 0;) {
    const bool exercise =
        n > 0 && (style == ExerciseStyle::kAmerican ||
                  (style == ExerciseStyle::kBermudan && n % stride == 0));
    const double pu = disc * p;
    const double pd = disc * (1.0 - p);
    for (std::size_t j = 0; j <= n; ++j) {
      v[j] = pu * v[j + 1] + pd * v[j];
      spot[j] *= u;
      if (exercise) v[j] = std::max(v[j], payoff(spot[j]));
    }
  }
  return v[0];
}

}  // namespace cpp4q::glasserman::ch08
