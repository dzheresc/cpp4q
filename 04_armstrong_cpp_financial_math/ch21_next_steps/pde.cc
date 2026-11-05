#include "ch21_next_steps/pde.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cpp4q::armstrong::ch21 {

void SolveTridiagonal(std::span<const double> a, std::span<const double> b,
                      std::span<const double> c, std::span<double> d) {
  const std::size_t n = d.size();
  if (a.size() != n || b.size() != n || c.size() != n) {
    throw std::invalid_argument("tridiagonal sizes differ");
  }
  if (n == 0) return;
  std::vector<double> c_prime(n);
  double denom = b[0];
  c_prime[0] = c[0] / denom;
  d[0] /= denom;
  for (std::size_t i = 1; i < n; ++i) {
    denom = b[i] - a[i] * c_prime[i - 1];
    c_prime[i] = c[i] / denom;
    d[i] = (d[i] - a[i] * d[i - 1]) / denom;
  }
  for (std::size_t i = n - 1; i-- > 0;) d[i] -= c_prime[i] * d[i + 1];
}

PdeResult PricePde(PayoffType type, ExerciseStyle style, double strike,
                   double maturity, double spot, double vol, double rate,
                   const PdeSettings& settings) {
  if (!(strike > 0 && maturity > 0 && spot > 0 && vol > 0)) {
    throw std::invalid_argument("PDE needs positive inputs");
  }
  const int n = settings.n_space;
  const int m = settings.n_time;
  if (n < 4 || m < 1) throw std::invalid_argument("grid too small");
  // Put the spot exactly on node j0, so price and Greeks are read off the
  // grid without interpolation.
  const double s_target = settings.s_max_multiple * std::max(spot, strike);
  const long j0 = std::lround(n * spot / s_target);
  if (j0 < 1 || j0 > n - 1) {
    throw std::invalid_argument("spot too close to the grid boundary");
  }
  const double h = spot / static_cast<double>(j0);
  const double s_max = n * h;
  const double dt = maturity / m;
  const auto un = static_cast<std::size_t>(n);
  const bool call = type == PayoffType::kCall;
  const bool american = style == ExerciseStyle::kAmerican;

  std::vector<double> payoff(un + 1);
  for (std::size_t i = 0; i <= un; ++i) {
    const double s = static_cast<double>(i) * h;
    payoff[i] = call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0);
  }
  // Initial values: the payoff averaged over each node's cell
  // [S_i - h/2, S_i + h/2]. It differs from the nodal payoff only in the
  // cell containing the strike, and removes the error that otherwise
  // depends on where the kink falls relative to the nodes.
  std::vector<double> v = payoff;
  {
    const auto i = static_cast<std::size_t>(std::lround(strike / h));
    if (i >= 1 && i < un) {
      const double lo_s = (static_cast<double>(i) - 0.5) * h;
      const double hi_s = lo_s + h;
      // Integral of (s - K)^+ over [lo_s, hi_s] (K is inside the cell).
      const double above = 0.5 * (hi_s - strike) * (hi_s - strike);
      const double below = 0.5 * (strike - lo_s) * (strike - lo_s);
      v[i] = (call ? above : below) / h;
    }
  }

  // L V at node i = lo_i V_{i-1} + mid_i V_i + up_i V_{i+1}, S_i = i h.
  std::vector<double> lo(un + 1), mid(un + 1), up(un + 1);
  for (std::size_t i = 1; i < un; ++i) {
    const double x = static_cast<double>(i);
    const double diff = 0.5 * vol * vol * x * x;
    const double conv = 0.5 * rate * x;
    lo[i] = diff - conv;
    mid[i] = -2 * diff - rate;
    up[i] = diff + conv;
  }

  const std::size_t interior = un - 1;
  std::vector<double> a(interior), b(interior), c(interior), rhs(interior);
  for (int step = 1; step <= m; ++step) {
    const double tau = step * dt;
    const double theta = step <= settings.rannacher_steps ? 1.0 : 0.5;
    // Boundary values at the new time level.
    double v0, vn;
    if (call) {
      v0 = 0.0;
      vn = american ? s_max - strike : s_max - strike * std::exp(-rate * tau);
    } else {
      v0 = american ? strike : strike * std::exp(-rate * tau);
      vn = 0.0;
    }
    for (std::size_t i = 1; i < un; ++i) {
      const std::size_t k = i - 1;
      const double e = (1 - theta) * dt;
      rhs[k] = v[i] + e * (lo[i] * v[i - 1] + mid[i] * v[i] + up[i] * v[i + 1]);
      a[k] = -theta * dt * lo[i];
      b[k] = 1 - theta * dt * mid[i];
      c[k] = -theta * dt * up[i];
    }
    rhs[0] -= a[0] * v0;
    rhs[interior - 1] -= c[interior - 1] * vn;

    if (!american) {
      SolveTridiagonal(a, b, c, rhs);
      for (std::size_t k = 0; k < interior; ++k) v[k + 1] = rhs[k];
    } else {
      // Projected SOR from the previous time level's values.
      v[0] = v0;
      v[un] = vn;
      for (int iter = 0; iter < 10'000; ++iter) {
        double change = 0.0;
        for (std::size_t k = 0; k < interior; ++k) {
          const std::size_t i = k + 1;
          const double left = k > 0 ? a[k] * v[i - 1] : 0.0;
          const double right = k + 1 < interior ? c[k] * v[i + 1] : 0.0;
          const double gs = (rhs[k] - left - right) / b[k];
          const double next =
              std::max(payoff[i], v[i] + settings.sor_omega * (gs - v[i]));
          change = std::max(change, std::abs(next - v[i]));
          v[i] = next;
        }
        if (change < settings.sor_tolerance) break;
      }
    }
    v[0] = v0;
    v[un] = vn;
  }

  const auto j = static_cast<std::size_t>(j0);
  return {v[j], (v[j + 1] - v[j - 1]) / (2 * h),
          (v[j + 1] - 2 * v[j] + v[j - 1]) / (h * h)};
}

}  // namespace cpp4q::armstrong::ch21
