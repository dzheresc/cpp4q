#include "ch02_random_numbers/distributions.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace cpp4q::glasserman::ch02 {

double SampleExponential(Sampler& s, double rate) {
  return -std::log(s.Uniform()) / rate;
}

std::uint64_t SamplePoisson(Sampler& s, double mean) {
  if (mean < 0.0) throw std::invalid_argument("Poisson mean must be >= 0");
  if (mean == 0.0) return 0;
  // exp(-mean) underflows past ~700; a sum of independent Poissons is
  // Poisson, so split large means into chunks.
  constexpr double kChunk = 100.0;
  if (mean > kChunk) {
    const double pieces = std::ceil(mean / kChunk);
    std::uint64_t total = 0;
    for (double i = 0; i < pieces; ++i)
      total += SamplePoisson(s, mean / pieces);
    return total;
  }
  // Inversion by sequential search: smallest n with F(n) >= u.
  const double u = s.Uniform();
  double p = std::exp(-mean);
  double cdf = p;
  std::uint64_t n = 0;
  while (u > cdf) {
    ++n;
    p *= mean / static_cast<double>(n);
    cdf += p;
    if (p == 0.0 && cdf < u) break;  // rounding left a sliver above 1 - eps
  }
  return n;
}

double SampleGamma(Sampler& s, double shape, double scale) {
  if (shape <= 0.0) throw std::invalid_argument("gamma shape must be > 0");
  if (shape < 1.0) {
    // G(a) = G(a + 1) * U^(1/a).
    return SampleGamma(s, shape + 1.0, scale) *
           std::pow(s.Uniform(), 1.0 / shape);
  }
  // Marsaglia-Tsang (2000): acceptance-rejection with a transformed normal.
  const double d = shape - 1.0 / 3.0;
  const double c = 1.0 / std::sqrt(9.0 * d);
  while (true) {
    double z, v;
    do {
      z = s.Normal();
      v = 1.0 + c * z;
    } while (v <= 0.0);
    v = v * v * v;
    const double u = s.Uniform();
    if (u < 1.0 - 0.0331 * z * z * z * z) return d * v * scale;
    if (std::log(u) < 0.5 * z * z + d * (1.0 - v + std::log(v))) {
      return d * v * scale;
    }
  }
}

double SampleChiSquare(Sampler& s, double dof) {
  return SampleGamma(s, 0.5 * dof, 2.0);
}

double SampleNoncentralChiSquare(Sampler& s, double dof, double noncentrality) {
  // Glasserman (3.71): chi'^2_d(lambda) = chi^2_{d + 2N}, N ~
  // Poisson(lambda/2).
  const auto n = SamplePoisson(s, 0.5 * noncentrality);
  return SampleChiSquare(s, dof + 2.0 * static_cast<double>(n));
}

std::vector<double> CholeskyFactor(std::span<const double> a, std::size_t n) {
  if (a.size() != n * n) throw std::invalid_argument("matrix size mismatch");
  std::vector<double> l(n * n, 0.0);
  for (std::size_t j = 0; j < n; ++j) {
    double diag = a[j * n + j];
    for (std::size_t k = 0; k < j; ++k) diag -= l[j * n + k] * l[j * n + k];
    if (!(diag > 0.0)) {
      throw std::invalid_argument("matrix is not positive definite");
    }
    const double ljj = std::sqrt(diag);
    l[j * n + j] = ljj;
    for (std::size_t i = j + 1; i < n; ++i) {
      if (std::abs(a[i * n + j] - a[j * n + i]) > 1e-12) {
        throw std::invalid_argument("matrix is not symmetric");
      }
      double v = a[i * n + j];
      for (std::size_t k = 0; k < j; ++k) v -= l[i * n + k] * l[j * n + k];
      l[i * n + j] = v / ljj;
    }
  }
  return l;
}

MultivariateNormal::MultivariateNormal(std::vector<double> mean,
                                       std::span<const double> covariance)
    : mean_(std::move(mean)),
      chol_(CholeskyFactor(covariance, mean_.size())),
      z_(mean_.size()) {}

void MultivariateNormal::Sample(Sampler& s, std::span<double> out) {
  s.FillNormal(z_);
  Transform(z_, out);
}

void MultivariateNormal::Transform(std::span<const double> z,
                                   std::span<double> out) const {
  const std::size_t n = mean_.size();
  if (z.size() != n || out.size() != n) {
    throw std::invalid_argument("dimension mismatch");
  }
  for (std::size_t i = 0; i < n; ++i) {
    double v = mean_[i];
    for (std::size_t k = 0; k <= i; ++k) v += chol_[i * n + k] * z[k];
    out[i] = v;
  }
}

}  // namespace cpp4q::glasserman::ch02
