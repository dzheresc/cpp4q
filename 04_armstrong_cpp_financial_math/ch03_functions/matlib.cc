#include "ch03_functions/matlib.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cpp4q::armstrong::ch03 {
namespace {

constexpr double kInvSqrt2Pi = 0.39894228040143267794;

struct D12 {
  double d1;
  double d2;
};

D12 ComputeD12(double strike, double maturity, double spot, double vol,
               double rate) {
  if (!(strike > 0 && maturity > 0 && spot > 0 && vol > 0)) {
    throw std::invalid_argument(
        "Black-Scholes needs positive strike, maturity, spot and vol");
  }
  const double sd = vol * std::sqrt(maturity);
  const double d1 =
      (std::log(spot / strike) + (rate + 0.5 * vol * vol) * maturity) / sd;
  return {d1, d1 - sd};
}

// Acklam's coefficients.
constexpr double kA[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                         -2.759285104469687e+02, 1.383577518672690e+02,
                         -3.066479806614716e+01, 2.506628277459239e+00};
constexpr double kB[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                         -1.556989798598866e+02, 6.680131188771972e+01,
                         -1.328068155288572e+01};
constexpr double kC[] = {-7.784894002430293e-03, -3.223964580411365e-01,
                         -2.400758277161838e+00, -2.549732539343734e+00,
                         4.374664141464968e+00,  2.938163982698783e+00};
constexpr double kD[] = {7.784695709041462e-03, 3.224671290700398e-01,
                         2.445134137142996e+00, 3.754408661907416e+00};
constexpr double kLow = 0.02425;

double AcklamTail(double q) {  // q = sqrt(-2 log p), p in the lower tail
  return (((((kC[0] * q + kC[1]) * q + kC[2]) * q + kC[3]) * q + kC[4]) * q +
          kC[5]) /
         ((((kD[0] * q + kD[1]) * q + kD[2]) * q + kD[3]) * q + 1);
}

}  // namespace

double NormPdf(double x) { return kInvSqrt2Pi * std::exp(-0.5 * x * x); }

double NormCdf(double x) {
  return 0.5 * std::erfc(-x * std::numbers::sqrt2 / 2);
}

double NormCdfPolynomial(double x) {
  constexpr double kP = 0.2316419;
  constexpr double kB1 = 0.319381530, kB2 = -0.356563782, kB3 = 1.781477937,
                   kB4 = -1.821255978, kB5 = 1.330274429;
  const double z = std::abs(x);
  const double t = 1.0 / (1.0 + kP * z);
  const double poly = t * (kB1 + t * (kB2 + t * (kB3 + t * (kB4 + t * kB5))));
  const double upper = NormPdf(z) * poly;  // P(Z > |x|)
  return x >= 0 ? 1.0 - upper : upper;
}

double NormInv(double p) {
  if (!(p > 0.0 && p < 1.0)) {
    throw std::domain_error("NormInv needs p in (0, 1)");
  }
  double x;
  if (p < kLow) {
    x = AcklamTail(std::sqrt(-2 * std::log(p)));
  } else if (p > 1 - kLow) {
    x = -AcklamTail(std::sqrt(-2 * std::log1p(-p)));
  } else {
    const double q = p - 0.5;
    const double r = q * q;
    x = (((((kA[0] * r + kA[1]) * r + kA[2]) * r + kA[3]) * r + kA[4]) * r +
         kA[5]) *
        q /
        (((((kB[0] * r + kB[1]) * r + kB[2]) * r + kB[3]) * r + kB[4]) * r +
         1);
  }
  // One Halley step on f(x) = N(x) - p. Use the upper tail for p > 1/2 so
  // the residual is not swamped by rounding of values near 1.
  const double e = p <= 0.5 ? NormCdf(x) - p : (1 - p) - NormCdf(-x);
  const double u = e / NormPdf(x);
  return x - u / (1 + 0.5 * x * u);
}

double BlackScholesCallPrice(double strike, double maturity, double spot,
                             double vol, double rate) {
  const auto [d1, d2] = ComputeD12(strike, maturity, spot, vol, rate);
  return spot * NormCdf(d1) -
         strike * std::exp(-rate * maturity) * NormCdf(d2);
}

double BlackScholesPutPrice(double strike, double maturity, double spot,
                            double vol, double rate) {
  const auto [d1, d2] = ComputeD12(strike, maturity, spot, vol, rate);
  return strike * std::exp(-rate * maturity) * NormCdf(-d2) -
         spot * NormCdf(-d1);
}

double BlackScholesCallDelta(double strike, double maturity, double spot,
                             double vol, double rate) {
  return NormCdf(ComputeD12(strike, maturity, spot, vol, rate).d1);
}

double BlackScholesGamma(double strike, double maturity, double spot,
                         double vol, double rate) {
  const double d1 = ComputeD12(strike, maturity, spot, vol, rate).d1;
  return NormPdf(d1) / (spot * vol * std::sqrt(maturity));
}

double BlackScholesVega(double strike, double maturity, double spot,
                        double vol, double rate) {
  const double d1 = ComputeD12(strike, maturity, spot, vol, rate).d1;
  return spot * NormPdf(d1) * std::sqrt(maturity);
}

std::optional<std::pair<double, double>> SolveQuadratic(double a, double b,
                                                        double c) {
  if (a == 0.0) return std::nullopt;
  const double disc = b * b - 4 * a * c;
  if (disc < 0) return std::nullopt;
  // q has the sign of b, so b + sign(b) sqrt(disc) never cancels.
  const double q = -0.5 * (b + std::copysign(std::sqrt(disc), b));
  double x1 = q / a;
  double x2 = q != 0.0 ? c / q : x1;
  if (x1 > x2) std::swap(x1, x2);
  return std::pair{x1, x2};
}

}  // namespace cpp4q::armstrong::ch03
