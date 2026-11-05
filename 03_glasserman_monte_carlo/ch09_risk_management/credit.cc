#include "ch09_risk_management/credit.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "ch01_foundations/black_scholes.h"
#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch09 {

GaussianCopulaPortfolio::GaussianCopulaPortfolio(std::vector<Obligor> obligors)
    : obligors_(std::move(obligors)) {
  for (const auto& o : obligors_) {
    if (!(o.default_prob > 0.0 && o.default_prob < 1.0) ||
        !(o.loading >= 0.0 && o.loading < 1.0) || !(o.exposure > 0.0)) {
      throw std::invalid_argument("bad obligor");
    }
    thresholds_.push_back(ch02::InverseNormalCdfAccurate(o.default_prob));
    idio_sd_.push_back(std::sqrt(1.0 - o.loading * o.loading));
  }
}

double GaussianCopulaPortfolio::ExpectedLoss() const {
  double el = 0.0;
  for (const auto& o : obligors_) el += o.default_prob * o.exposure;
  return el;
}

double GaussianCopulaPortfolio::ConditionalDefaultProb(std::size_t k,
                                                       double y) const {
  return ch01::NormalCdf((thresholds_[k] - obligors_[k].loading * y) /
                         idio_sd_[k]);
}

ch01::Estimate GaussianCopulaPortfolio::TailProbability(
    double x, std::size_t paths, std::uint64_t seed) const {
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    const double y = s.Normal();
    double loss = 0.0;
    for (std::size_t k = 0; k < obligors_.size(); ++k) {
      const double latent = obligors_[k].loading * y + idio_sd_[k] * s.Normal();
      if (latent < thresholds_[k]) loss += obligors_[k].exposure;
    }
    st.Add(loss > x ? 1.0 : 0.0);
  }
  return st.ToEstimate();
}

double GaussianCopulaPortfolio::Psi(const std::vector<double>& p,
                                    double theta) const {
  double psi = 0.0;
  for (std::size_t k = 0; k < p.size(); ++k) {
    psi += std::log1p(p[k] * std::expm1(theta * obligors_[k].exposure));
  }
  return psi;
}

double GaussianCopulaPortfolio::SolveTheta(const std::vector<double>& p,
                                           double x) const {
  // psi'(theta) = sum c q(theta) is increasing and convex-then-concave in
  // theta; psi''(theta) = sum c^2 q (1 - q). Newton from a bracket, falling
  // back to bisection when a step leaves it. The first version bisected
  // 100 times per path: with its exp() calls, ~80% of the IS run.
  auto mean_and_slope = [&](double theta, double& slope) {
    double m = 0.0;
    slope = 0.0;
    for (std::size_t k = 0; k < p.size(); ++k) {
      const double c = obligors_[k].exposure;
      const double e = std::exp(theta * c);
      const double q = p[k] * e / (1.0 + p[k] * (e - 1.0));
      m += c * q;
      slope += c * c * q * (1.0 - q);
    }
    return m;
  };
  double slope;
  if (mean_and_slope(0.0, slope) >= x) return 0.0;
  double total = 0.0;
  for (const auto& o : obligors_) total += o.exposure;
  if (x >= total)
    throw std::invalid_argument("threshold beyond total exposure");
  double lo = 0.0;
  double hi = 1.0;
  while (mean_and_slope(hi, slope) < x) {
    lo = hi;
    hi *= 2.0;
  }
  double theta = 0.5 * (lo + hi);
  for (int it = 0; it < 100; ++it) {
    const double f = mean_and_slope(theta, slope) - x;
    if (std::abs(f) <= 1e-10 * x) break;
    (f < 0.0 ? lo : hi) = theta;
    const double next = theta - f / slope;
    theta = (slope > 0.0 && next > lo && next < hi) ? next : 0.5 * (lo + hi);
    if (hi - lo <= 1e-14 * hi) break;
  }
  return theta;
}

ch01::Estimate GaussianCopulaPortfolio::TailProbabilityIs(
    double x, double shift, std::size_t paths, std::uint64_t seed) const {
  const std::size_t n = obligors_.size();
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  std::vector<double> p(n);
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    const double y = shift + s.Normal();
    for (std::size_t k = 0; k < n; ++k) p[k] = ConditionalDefaultProb(k, y);
    const double theta = SolveTheta(p, x);
    double loss = 0.0;
    for (std::size_t k = 0; k < n; ++k) {
      const double c = obligors_[k].exposure;
      const double e = std::exp(theta * c);
      const double q = p[k] * e / (1.0 + p[k] * (e - 1.0));
      if (s.Uniform() < q) loss += c;
    }
    if (loss > x) {
      const double log_lr =
          -theta * loss + Psi(p, theta) - shift * y + 0.5 * shift * shift;
      st.Add(std::exp(log_lr));
    } else {
      st.Add(0.0);
    }
  }
  return st.ToEstimate();
}

double GaussianCopulaPortfolio::FindFactorShift(double x) const {
  const std::size_t n = obligors_.size();
  std::vector<double> p(n);
  double best_y = 0.0;
  double best = -1e300;
  for (double y = -8.0; y <= 0.0; y += 0.02) {
    for (std::size_t k = 0; k < n; ++k) p[k] = ConditionalDefaultProb(k, y);
    const double theta = SolveTheta(p, x);
    const double f = Psi(p, theta) - theta * x - 0.5 * y * y;
    if (f > best) {
      best = f;
      best_y = y;
    }
  }
  return best_y;
}

}  // namespace cpp4q::glasserman::ch09
