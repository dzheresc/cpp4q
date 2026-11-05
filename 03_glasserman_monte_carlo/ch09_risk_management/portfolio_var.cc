#include "ch09_risk_management/portfolio_var.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

#include "ch02_random_numbers/distributions.h"
#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch09 {
namespace {

ch01::BlackScholesInputs Inputs(const Market& m, const OptionPosition& p,
                                double spot, double elapsed) {
  return {.spot = spot,
          .strike = p.strike,
          .rate = m.rate,
          .vol = m.vols[p.asset],
          .maturity = p.maturity - elapsed,
          .dividend = 0.0};
}

// Linearised loss in z: L ~ -sum_j delta_j S_j vol_j sqrt(dt) (C z)_j =
// b . z. Returns b.
std::vector<double> LinearLossDirection(const OptionPortfolio& book,
                                        double horizon) {
  const auto& chol = book.correlation_factor();
  const auto g = book.Sensitivities();
  const std::size_t n = book.assets();
  const double sqrt_dt = std::sqrt(horizon);
  std::vector<double> b(n, 0.0);
  for (std::size_t i = 0; i < n; ++i) {
    const double w =
        -g.delta[i] * book.market().spots[i] * book.market().vols[i] * sqrt_dt;
    for (std::size_t k = 0; k <= i; ++k) b[k] += w * chol[i * n + k];
  }
  return b;
}

}  // namespace

OptionPortfolio::OptionPortfolio(Market market,
                                 std::vector<OptionPosition> positions)
    : market_(std::move(market)),
      positions_(std::move(positions)),
      chol_(ch02::CholeskyFactor(market_.correlation, market_.spots.size())),
      value0_(0.0) {
  if (market_.vols.size() != market_.spots.size()) {
    throw std::invalid_argument("one vol per asset");
  }
  for (const auto& p : positions_) {
    if (p.asset >= market_.spots.size()) {
      throw std::invalid_argument("position on unknown asset");
    }
  }
  value0_ = Value(market_.spots, 0.0);
}

double OptionPortfolio::Value(std::span<const double> spots,
                              double elapsed) const {
  double v = 0.0;
  for (const auto& p : positions_) {
    v += p.quantity * ch01::BlackScholesPrice(
                          p.type, Inputs(market_, p, spots[p.asset], elapsed));
  }
  return v;
}

DeltaGammaTheta OptionPortfolio::Sensitivities() const {
  DeltaGammaTheta g{std::vector<double>(assets(), 0.0),
                    std::vector<double>(assets(), 0.0), 0.0};
  constexpr double kDt = 1.0 / 3650.0;  // theta by a tenth-of-a-day bump
  for (const auto& p : positions_) {
    const auto in = Inputs(market_, p, market_.spots[p.asset], 0.0);
    g.delta[p.asset] += p.quantity * ch01::BlackScholesDelta(p.type, in);
    g.gamma[p.asset] += p.quantity * ch01::BlackScholesGamma(in);
    const auto later = Inputs(market_, p, market_.spots[p.asset], kDt);
    g.theta += p.quantity *
               (ch01::BlackScholesPrice(p.type, later) -
                ch01::BlackScholesPrice(p.type, in)) /
               kDt;
  }
  return g;
}

double OptionPortfolio::ApproxLoss(const DeltaGammaTheta& g,
                                   std::span<const double> ds, double horizon) {
  double dv = g.theta * horizon;
  for (std::size_t j = 0; j < ds.size(); ++j) {
    dv += g.delta[j] * ds[j] + 0.5 * g.gamma[j] * ds[j] * ds[j];
  }
  return -dv;
}

void OptionPortfolio::Scenario(std::span<const double> z, double horizon,
                               std::span<double> spots) const {
  const std::size_t n = assets();
  const double sqrt_dt = std::sqrt(horizon);
  for (std::size_t i = 0; i < n; ++i) {
    double x = 0.0;
    for (std::size_t k = 0; k <= i; ++k) x += chol_[i * n + k] * z[k];
    const double vol = market_.vols[i];
    spots[i] = market_.spots[i] *
               std::exp(vol * sqrt_dt * x - 0.5 * vol * vol * horizon);
  }
}

std::vector<double> SimulateLosses(const OptionPortfolio& book, double horizon,
                                   std::size_t paths, bool delta_gamma,
                                   std::uint64_t seed) {
  const std::size_t n = book.assets();
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  const DeltaGammaTheta g = book.Sensitivities();
  std::vector<double> z(n), spots(n), ds(n), losses(paths);
  for (std::size_t i = 0; i < paths; ++i) {
    s.FillNormal(z);
    book.Scenario(z, horizon, spots);
    if (delta_gamma) {
      for (std::size_t j = 0; j < n; ++j)
        ds[j] = spots[j] - book.market().spots[j];
      losses[i] = OptionPortfolio::ApproxLoss(g, ds, horizon);
    } else {
      losses[i] = book.Loss(spots, horizon);
    }
  }
  return losses;
}

double ValueAtRisk(std::vector<double> losses, double alpha) {
  if (losses.empty() || !(alpha > 0.0 && alpha < 1.0)) {
    throw std::invalid_argument("need losses and alpha in (0, 1)");
  }
  const auto k = static_cast<std::size_t>(std::ceil(
                     (1.0 - alpha) * static_cast<double>(losses.size()))) -
                 1;
  std::nth_element(losses.begin(),
                   losses.begin() + static_cast<std::ptrdiff_t>(k),
                   losses.end());
  return losses[k];
}

double ExpectedShortfall(std::vector<double> losses, double alpha) {
  const double var = ValueAtRisk(losses, alpha);
  double sum = 0.0;
  std::size_t count = 0;
  for (double l : losses) {
    if (l >= var) {
      sum += l;
      ++count;
    }
  }
  return sum / static_cast<double>(count);
}

ch01::Estimate TailProbability(const OptionPortfolio& book, double horizon,
                               double threshold, std::size_t paths,
                               std::uint64_t seed) {
  const std::size_t n = book.assets();
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  std::vector<double> z(n), spots(n);
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    s.FillNormal(z);
    book.Scenario(z, horizon, spots);
    st.Add(book.Loss(spots, horizon) > threshold ? 1.0 : 0.0);
  }
  return st.ToEstimate();
}

SymmetricEigen JacobiEigen(std::vector<double> a, std::size_t n) {
  if (a.size() != n * n) throw std::invalid_argument("matrix size mismatch");
  std::vector<double> v(n * n, 0.0);
  for (std::size_t i = 0; i < n; ++i) v[i * n + i] = 1.0;
  for (int sweep = 0; sweep < 100; ++sweep) {
    double off = 0.0;
    for (std::size_t p = 0; p < n; ++p) {
      for (std::size_t q = p + 1; q < n; ++q)
        off += a[p * n + q] * a[p * n + q];
    }
    if (off < 1e-30) break;
    for (std::size_t p = 0; p < n; ++p) {
      for (std::size_t q = p + 1; q < n; ++q) {
        const double apq = a[p * n + q];
        if (std::abs(apq) < 1e-300) continue;
        // Rotation angle that zeroes a[p][q].
        const double tau = (a[q * n + q] - a[p * n + p]) / (2.0 * apq);
        const double t = (tau >= 0 ? 1.0 : -1.0) /
                         (std::abs(tau) + std::sqrt(1.0 + tau * tau));
        const double c = 1.0 / std::sqrt(1.0 + t * t);
        const double sn = t * c;
        for (std::size_t k = 0; k < n; ++k) {  // columns p, q
          const double akp = a[k * n + p];
          const double akq = a[k * n + q];
          a[k * n + p] = c * akp - sn * akq;
          a[k * n + q] = sn * akp + c * akq;
        }
        for (std::size_t k = 0; k < n; ++k) {  // rows p, q
          const double apk = a[p * n + k];
          const double aqk = a[q * n + k];
          a[p * n + k] = c * apk - sn * aqk;
          a[q * n + k] = sn * apk + c * aqk;
        }
        for (std::size_t k = 0; k < n; ++k) {
          const double vkp = v[k * n + p];
          const double vkq = v[k * n + q];
          v[k * n + p] = c * vkp - sn * vkq;
          v[k * n + q] = sn * vkp + c * vkq;
        }
      }
    }
  }
  SymmetricEigen out{std::vector<double>(n), std::move(v)};
  for (std::size_t i = 0; i < n; ++i) out.values[i] = a[i * n + i];
  return out;
}

DiagonalQuadratic DiagonalizeDeltaGamma(const OptionPortfolio& book,
                                        double horizon) {
  // dS_i ~ S_i vol_i sqrt(dt) (A z)_i = (D A z)_i, A the correlation factor.
  // L ~ -theta dt - delta' D A z - 1/2 z' A' D Gamma D A z.
  const std::size_t n = book.assets();
  const auto g = book.Sensitivities();
  const auto& a = book.correlation_factor();
  const double sqrt_dt = std::sqrt(horizon);
  std::vector<double> d(n);
  for (std::size_t i = 0; i < n; ++i) {
    d[i] = book.market().spots[i] * book.market().vols[i] * sqrt_dt;
  }
  // m = -1/2 A' D Gamma D A (Gamma is diagonal).
  std::vector<double> m(n * n, 0.0);
  for (std::size_t r = 0; r < n; ++r) {
    for (std::size_t c = 0; c < n; ++c) {
      double v = 0.0;
      for (std::size_t i = 0; i < n; ++i) {
        v += a[i * n + r] * d[i] * g.gamma[i] * d[i] * a[i * n + c];
      }
      m[r * n + c] = -0.5 * v;
    }
  }
  // Linear term in z: b_z = -A' D delta.
  std::vector<double> bz(n, 0.0);
  for (std::size_t r = 0; r < n; ++r) {
    for (std::size_t i = 0; i < n; ++i) {
      bz[r] -= a[i * n + r] * d[i] * g.delta[i];
    }
  }
  SymmetricEigen eig = JacobiEigen(std::move(m), n);
  DiagonalQuadratic q{-g.theta * horizon, std::vector<double>(n, 0.0),
                      std::move(eig.values), std::move(eig.vectors)};
  for (std::size_t j = 0; j < n; ++j) {  // b_y = V' b_z
    for (std::size_t r = 0; r < n; ++r) {
      q.linear[j] += q.rotation[r * n + j] * bz[r];
    }
  }
  return q;
}

namespace {

// psi(theta) = log E[exp(theta (Q - constant))] and its derivative.
double TwistPsi(const DiagonalQuadratic& q, double theta) {
  double psi = 0.0;
  for (std::size_t j = 0; j < q.linear.size(); ++j) {
    const double one_minus = 1.0 - 2.0 * theta * q.quadratic[j];
    psi += 0.5 * theta * theta * q.linear[j] * q.linear[j] / one_minus -
           0.5 * std::log(one_minus);
  }
  return psi;
}

double TwistPsiPrime(const DiagonalQuadratic& q, double theta) {
  double d = 0.0;
  for (std::size_t j = 0; j < q.linear.size(); ++j) {
    const double l = q.quadratic[j];
    const double b = q.linear[j];
    const double one_minus = 1.0 - 2.0 * theta * l;
    d += theta * b * b * (1.0 - theta * l) / (one_minus * one_minus) +
         l / one_minus;
  }
  return d;
}

// theta >= 0 with psi'(theta) = target, inside 1 - 2 theta lambda_j > 0.
double SolveTwist(const DiagonalQuadratic& q, double target) {
  if (TwistPsiPrime(q, 0.0) >= target) return 0.0;
  double max_lambda = 0.0;
  for (double l : q.quadratic) max_lambda = std::max(max_lambda, l);
  double lo = 0.0;
  double hi = max_lambda > 0.0 ? 0.5 / max_lambda : 1.0;
  if (max_lambda <= 0.0) {
    while (TwistPsiPrime(q, hi) < target) hi *= 2.0;
  }
  for (int it = 0; it < 200; ++it) {
    const double mid = 0.5 * (lo + hi);
    (TwistPsiPrime(q, mid) < target ? lo : hi) = mid;
  }
  return 0.5 * (lo + hi);
}

}  // namespace

ch01::Estimate TailProbabilityIs(const OptionPortfolio& book, double horizon,
                                 double threshold, std::size_t paths,
                                 std::uint64_t seed, IsMethod method) {
  const std::size_t n = book.assets();
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  std::vector<double> y(n), z(n), spots(n);
  ch01::RunningStats st;
  if (method == IsMethod::kMeanShift) {
    const auto b = LinearLossDirection(book, horizon);
    const double norm2 = std::inner_product(b.begin(), b.end(), b.begin(), 0.0);
    std::vector<double> mu(n, 0.0);
    if (norm2 > 0.0) {
      // Closest point to the origin on {z : b . z = threshold - theta part}.
      const double target = threshold + book.Sensitivities().theta * horizon;
      for (std::size_t j = 0; j < n; ++j) mu[j] = target * b[j] / norm2;
    }
    const double half_mu2 =
        0.5 * std::inner_product(mu.begin(), mu.end(), mu.begin(), 0.0);
    for (std::size_t i = 0; i < paths; ++i) {
      s.FillNormal(z);
      double mu_z = 0.0;
      for (std::size_t j = 0; j < n; ++j) {
        z[j] += mu[j];
        mu_z += mu[j] * z[j];
      }
      book.Scenario(z, horizon, spots);
      const bool hit = book.Loss(spots, horizon) > threshold;
      st.Add(hit ? std::exp(-mu_z + half_mu2) : 0.0);
    }
    return st.ToEstimate();
  }

  const DiagonalQuadratic q = DiagonalizeDeltaGamma(book, horizon);
  const double theta = SolveTwist(q, threshold - q.constant);
  const double psi = TwistPsi(q, theta);
  // Under the twist, Y_j ~ N(theta b_j / (1 - 2 theta l_j), 1 / (1 - 2 theta
  // l_j)).
  std::vector<double> mean(n), sd(n);
  for (std::size_t j = 0; j < n; ++j) {
    const double one_minus = 1.0 - 2.0 * theta * q.quadratic[j];
    mean[j] = theta * q.linear[j] / one_minus;
    sd[j] = 1.0 / std::sqrt(one_minus);
  }
  for (std::size_t i = 0; i < paths; ++i) {
    double quad = 0.0;  // Q - constant
    for (std::size_t j = 0; j < n; ++j) {
      y[j] = mean[j] + sd[j] * s.Normal();
      quad += q.linear[j] * y[j] + q.quadratic[j] * y[j] * y[j];
    }
    for (std::size_t r = 0; r < n; ++r) {
      double v = 0.0;
      for (std::size_t j = 0; j < n; ++j) v += q.rotation[r * n + j] * y[j];
      z[r] = v;
    }
    book.Scenario(z, horizon, spots);
    const bool hit = book.Loss(spots, horizon) > threshold;
    st.Add(hit ? std::exp(-theta * quad + psi) : 0.0);
  }
  return st.ToEstimate();
}

}  // namespace cpp4q::glasserman::ch09
