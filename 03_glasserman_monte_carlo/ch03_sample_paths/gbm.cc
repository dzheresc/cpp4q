#include "ch03_sample_paths/gbm.h"

#include <cmath>
#include <stdexcept>
#include <utility>

#include "ch02_random_numbers/distributions.h"

namespace cpp4q::glasserman::ch03 {

void GbmFromBrownian(const GbmParams& p, std::span<const double> times,
                     std::span<const double> w, std::span<double> s) {
  if (w.size() != times.size() || s.size() != times.size()) {
    throw std::invalid_argument("size mismatch");
  }
  const double mu = p.drift - 0.5 * p.vol * p.vol;
  for (std::size_t i = 0; i < times.size(); ++i) {
    s[i] = p.spot * std::exp(mu * times[i] + p.vol * w[i]);
  }
}

void GbmPath(const GbmParams& p, std::span<const double> times,
             std::span<const double> z, std::span<double> s) {
  if (z.size() != times.size() || s.size() != times.size()) {
    throw std::invalid_argument("size mismatch");
  }
  const double mu = p.drift - 0.5 * p.vol * p.vol;
  double prev_t = 0.0;
  double x = std::log(p.spot);
  for (std::size_t i = 0; i < times.size(); ++i) {
    const double dt = times[i] - prev_t;
    x += mu * dt + p.vol * std::sqrt(dt) * z[i];
    s[i] = std::exp(x);
    prev_t = times[i];
  }
}

MultiAssetGbm::MultiAssetGbm(std::vector<GbmParams> assets,
                             std::span<const double> correlation)
    : assets_(std::move(assets)),
      chol_(ch02::CholeskyFactor(correlation, assets_.size())) {}

void MultiAssetGbm::Terminal(double t, std::span<const double> z,
                             std::span<double> s) const {
  const std::size_t d = dim();
  if (z.size() != d || s.size() != d) {
    throw std::invalid_argument("size mismatch");
  }
  const double sqrt_t = std::sqrt(t);
  for (std::size_t i = 0; i < d; ++i) {
    double x = 0.0;
    for (std::size_t k = 0; k <= i; ++k) x += chol_[i * d + k] * z[k];
    const GbmParams& a = assets_[i];
    s[i] = a.spot *
           std::exp((a.drift - 0.5 * a.vol * a.vol) * t + a.vol * sqrt_t * x);
  }
}

void MultiAssetGbm::Path(std::span<const double> times,
                         std::span<const double> z, std::span<double> s) const {
  const std::size_t d = dim();
  const std::size_t n = times.size();
  if (z.size() != n * d || s.size() != n * d) {
    throw std::invalid_argument("size mismatch");
  }
  double prev_t = 0.0;
  for (std::size_t step = 0; step < n; ++step) {
    const double dt = times[step] - prev_t;
    const double sqrt_dt = std::sqrt(dt);
    for (std::size_t i = 0; i < d; ++i) {
      double x = 0.0;
      for (std::size_t k = 0; k <= i; ++k) {
        x += chol_[i * d + k] * z[step * d + k];
      }
      const GbmParams& a = assets_[i];
      const double prev = step == 0 ? a.spot : s[(step - 1) * d + i];
      s[step * d + i] = prev * std::exp((a.drift - 0.5 * a.vol * a.vol) * dt +
                                        a.vol * sqrt_dt * x);
    }
    prev_t = times[step];
  }
}

}  // namespace cpp4q::glasserman::ch03
