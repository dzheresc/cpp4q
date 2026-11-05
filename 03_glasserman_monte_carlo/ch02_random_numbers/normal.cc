#include "ch02_random_numbers/normal.h"

#include <cmath>
#include <numbers>

#include "ch01_foundations/black_scholes.h"

namespace cpp4q::glasserman::ch02 {

double InverseNormalCdfAccurate(double u) {
  const double x = InverseNormalCdf(u);
  // Halley's method on f(x) = Phi(x) - u, using the tail-accurate erfc.
  const double e = ch01::NormalCdf(x) - u;
  const double t =
      e * std::sqrt(2.0 * std::numbers::pi) * std::exp(0.5 * x * x);
  return x - t / (1.0 + 0.5 * x * t);
}

double Sampler::OtherNormal() {
  return method_ == NormalMethod::kBoxMuller ? BoxMuller() : Polar();
}

void Sampler::FillNormal(std::span<double> out) {
  if (method_ == NormalMethod::kInversion) {
    for (double& z : out) z = InverseNormalCdf(rng_.NextUniform());
  } else {
    for (double& z : out) z = OtherNormal();
  }
}

double Sampler::BoxMuller() {
  if (has_spare_) {
    has_spare_ = false;
    return spare_;
  }
  const double radius = std::sqrt(-2.0 * std::log(rng_.NextUniform()));
  const double angle = 2.0 * std::numbers::pi * rng_.NextUniform();
  spare_ = radius * std::sin(angle);
  has_spare_ = true;
  return radius * std::cos(angle);
}

double Sampler::Polar() {
  if (has_spare_) {
    has_spare_ = false;
    return spare_;
  }
  double v1, v2, s;
  do {
    v1 = 2.0 * rng_.NextUniform() - 1.0;
    v2 = 2.0 * rng_.NextUniform() - 1.0;
    s = v1 * v1 + v2 * v2;
  } while (s >= 1.0 || s == 0.0);
  const double scale = std::sqrt(-2.0 * std::log(s) / s);
  spare_ = v2 * scale;
  has_spare_ = true;
  return v1 * scale;
}

}  // namespace cpp4q::glasserman::ch02
