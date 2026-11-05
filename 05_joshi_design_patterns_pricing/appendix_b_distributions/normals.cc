#include "appendix_b_distributions/normals.h"

#include <cmath>
#include <limits>
#include <numbers>

namespace cpp4q::joshi {
namespace {

constexpr double kInvSqrt2Pi = 0.5 * std::numbers::inv_sqrtpi *
                               std::numbers::sqrt2;  // 1 / sqrt(2 pi)

// Beasley-Springer central region coefficients.
constexpr double kA[4] = {2.50662823884, -18.61500062529, 41.39119773534,
                          -25.44106049637};
constexpr double kB[4] = {-8.47351093090, 23.08336743743, -21.06224101826,
                          3.13082909833};
// Moro's Chebyshev tail coefficients.
constexpr double kC[9] = {0.3374754822726147, 0.9761690190917186,
                          0.1607979714918209, 0.0276438810333863,
                          0.0038405729373609, 0.0003951896511919,
                          0.0000321767881768, 0.0000002888167364,
                          0.0000003960315187};

}  // namespace

double NormalDensity(double x) { return kInvSqrt2Pi * std::exp(-0.5 * x * x); }

double CumulativeNormal(double x) {
  return 0.5 * std::erfc(-x * (0.5 * std::numbers::sqrt2));
}

double InverseCumulativeNormal(double u) {
  if (!(u >= 0.0 && u <= 1.0)) return std::numeric_limits<double>::quiet_NaN();
  if (u == 0.0) return -std::numeric_limits<double>::infinity();
  if (u == 1.0) return std::numeric_limits<double>::infinity();
  const double x = u - 0.5;
  if (std::fabs(x) < 0.42) {
    const double r = x * x;
    return x * (((kA[3] * r + kA[2]) * r + kA[1]) * r + kA[0]) /
           ((((kB[3] * r + kB[2]) * r + kB[1]) * r + kB[0]) * r + 1.0);
  }
  const double tail = x < 0.0 ? u : 1.0 - u;
  const double r = std::log(-std::log(tail));
  double s = kC[8];
  for (int i = 7; i >= 0; --i) s = s * r + kC[i];
  return x < 0.0 ? -s : s;
}

double InverseCumulativeNormalRefined(double u) {
  const double x = InverseCumulativeNormal(u);
  if (!std::isfinite(x)) return x;
  // Halley: f(x) = N(x) - u, f' = phi, f''/f' = -x.
  const double e = CumulativeNormal(x) - u;
  const double t = e / NormalDensity(x);
  return x - t / (1.0 + 0.5 * x * t);
}

}  // namespace cpp4q::joshi
