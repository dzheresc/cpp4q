#include "ch01_foundations/monte_carlo.h"

#include <cmath>
#include <stdexcept>

namespace cpp4q::glasserman::ch01 {

Estimate PriceEuropeanCall(const BlackScholesInputs& in, std::size_t paths,
                           std::uint64_t seed) {
  return PriceEuropean(in, CallPayoff{in.strike}, paths, seed);
}

Estimate PriceArithmeticAsianCall(const BlackScholesInputs& in, int fixings,
                                  std::size_t paths, std::uint64_t seed) {
  if (fixings < 1) throw std::invalid_argument("need at least one fixing");
  std::mt19937_64 rng(seed);
  std::normal_distribution<double> normal;
  const double dt = in.maturity / fixings;
  const double drift = (in.rate - in.dividend - 0.5 * in.vol * in.vol) * dt;
  const double sd = in.vol * std::sqrt(dt);
  const double df = std::exp(-in.rate * in.maturity);
  RunningStats stats;
  for (std::size_t i = 0; i < paths; ++i) {
    double log_s = std::log(in.spot);
    double sum = 0.0;
    for (int k = 0; k < fixings; ++k) {
      log_s += drift + sd * normal(rng);
      sum += std::exp(log_s);
    }
    stats.Add(df * std::max(sum / fixings - in.strike, 0.0));
  }
  return stats.ToEstimate();
}

std::vector<ConvergencePoint> ConvergenceStudy(const BlackScholesInputs& in,
                                               int min_log2, int max_log2,
                                               std::uint64_t seed) {
  const double exact = BlackScholesPrice(OptionType::kCall, in);
  std::vector<ConvergencePoint> out;
  for (int k = min_log2; k <= max_log2; ++k) {
    const std::size_t n = std::size_t{1} << k;
    const Estimate e =
        PriceEuropeanCall(in, n, seed + static_cast<unsigned>(k));
    out.push_back({n, std::abs(e.mean - exact), e.std_error});
  }
  return out;
}

double StdErrorSlope(const std::vector<ConvergencePoint>& points) {
  if (points.size() < 2) throw std::invalid_argument("need two points");
  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (const auto& p : points) {
    const double x = std::log(static_cast<double>(p.paths));
    const double y = std::log(p.std_error);
    sx += x;
    sy += y;
    sxx += x * x;
    sxy += x * y;
  }
  const auto n = static_cast<double>(points.size());
  return (n * sxy - sx * sy) / (n * sxx - sx * sx);
}

}  // namespace cpp4q::glasserman::ch01
