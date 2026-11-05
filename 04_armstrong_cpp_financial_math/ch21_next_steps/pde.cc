#include "ch21_next_steps/pde.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cpp4q::armstrong::ch21 {

namespace {

// (I - theta dt L) restricted to the interior nodes 1..n-1, with its
// elimination factors precomputed. Sub-diagonal a, diagonal b and
// super-diagonal c (a(0) and c(m-1) are the boundary couplings).
//
// Elimination order: the usual Thomas order (low S to high S) leaves a
// final substitution sweep from the highest node down; the reverse order
// leaves one from the lowest node up (`sweep_upward`). Brennan-Schwartz
// applies the early-exercise floor during that final sweep, which solves
// the American problem exactly when the sweep starts inside the exercise
// region: downward for calls (exercise at high S), upward for puts.
class TridiagonalFactor {
 public:
  TridiagonalFactor(const std::vector<double>& lo,
                    const std::vector<double>& mid,
                    const std::vector<double>& up, double dt, double theta,
                    bool sweep_upward)
      : sweep_upward_(sweep_upward) {
    const std::size_t m = lo.size() - 2;
    a_.resize(m);
    b_.resize(m);
    c_.resize(m);
    for (std::size_t k = 0; k < m; ++k) {
      a_[k] = -theta * dt * lo[k + 1];
      b_[k] = 1 - theta * dt * mid[k + 1];
      c_[k] = -theta * dt * up[k + 1];
    }
    inv_denom_.resize(m);
    ratio_.resize(m);
    if (!sweep_upward_) {
      inv_denom_[0] = 1 / b_[0];
      ratio_[0] = c_[0] * inv_denom_[0];
      for (std::size_t k = 1; k < m; ++k) {
        inv_denom_[k] = 1 / (b_[k] - a_[k] * ratio_[k - 1]);
        ratio_[k] = c_[k] * inv_denom_[k];
      }
    } else {
      inv_denom_[m - 1] = 1 / b_[m - 1];
      ratio_[m - 1] = a_[m - 1] * inv_denom_[m - 1];
      for (std::size_t k = m - 1; k-- > 0;) {
        inv_denom_[k] = 1 / (b_[k] - c_[k] * ratio_[k + 1]);
        ratio_[k] = a_[k] * inv_denom_[k];
      }
    }
  }

  double a(std::size_t k) const { return a_[k]; }
  double b(std::size_t k) const { return b_[k]; }
  double c(std::size_t k) const { return c_[k]; }

  // Solves in place.
  void Solve(std::span<double> d) const {
    Eliminate(d);
    const std::size_t m = d.size();
    if (!sweep_upward_) {
      for (std::size_t k = m - 1; k-- > 0;) d[k] -= ratio_[k] * d[k + 1];
    } else {
      for (std::size_t k = 1; k < m; ++k) d[k] -= ratio_[k] * d[k - 1];
    }
  }

  // Brennan-Schwartz: x = max(floor, back-substituted value), node by
  // node in the final sweep. d is destroyed.
  void SolveWithFloor(std::span<double> d, std::span<const double> floor,
                      std::span<double> x) const {
    Eliminate(d);
    const std::size_t m = d.size();
    if (!sweep_upward_) {
      x[m - 1] = std::max(floor[m - 1], d[m - 1]);
      for (std::size_t k = m - 1; k-- > 0;) {
        x[k] = std::max(floor[k], d[k] - ratio_[k] * x[k + 1]);
      }
    } else {
      x[0] = std::max(floor[0], d[0]);
      for (std::size_t k = 1; k < m; ++k) {
        x[k] = std::max(floor[k], d[k] - ratio_[k] * x[k - 1]);
      }
    }
  }

 private:
  void Eliminate(std::span<double> d) const {
    const std::size_t m = d.size();
    if (!sweep_upward_) {
      d[0] *= inv_denom_[0];
      for (std::size_t k = 1; k < m; ++k) {
        d[k] = (d[k] - a_[k] * d[k - 1]) * inv_denom_[k];
      }
    } else {
      d[m - 1] *= inv_denom_[m - 1];
      for (std::size_t k = m - 1; k-- > 0;) {
        d[k] = (d[k] - c_[k] * d[k + 1]) * inv_denom_[k];
      }
    }
  }

  bool sweep_upward_;
  std::vector<double> a_, b_, c_, inv_denom_, ratio_;
};

}  // namespace

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
  // The coefficients do not depend on time, so the two implicit matrices
  // (theta = 1 for the Rannacher steps, 1/2 after) are factorised once,
  // and each time step is only a forward and a backward sweep.
  const std::size_t interior = un - 1;
  std::vector<double> lo(un + 1), mid(un + 1), up(un + 1);
  for (std::size_t i = 1; i < un; ++i) {
    const double x = static_cast<double>(i);
    const double diff = 0.5 * vol * vol * x * x;
    const double conv = 0.5 * rate * x;
    lo[i] = diff - conv;
    mid[i] = -2 * diff - rate;
    up[i] = diff + conv;
  }
  const bool sweep_upward = american && !call;  // puts exercise at low S
  const TridiagonalFactor implicit(lo, mid, up, dt, 1.0, sweep_upward);
  const TridiagonalFactor crank_nicolson(lo, mid, up, dt, 0.5, sweep_upward);

  std::vector<double> rhs(interior);
  for (int step = 1; step <= m; ++step) {
    const double tau = step * dt;
    const bool rannacher = step <= settings.rannacher_steps;
    const TridiagonalFactor& f = rannacher ? implicit : crank_nicolson;
    // Boundary values at the new time level.
    double v0, vn;
    if (call) {
      v0 = 0.0;
      vn = american ? s_max - strike : s_max - strike * std::exp(-rate * tau);
    } else {
      v0 = american ? strike : strike * std::exp(-rate * tau);
      vn = 0.0;
    }
    if (rannacher) {
      for (std::size_t k = 0; k < interior; ++k) rhs[k] = v[k + 1];
    } else {
      const double e = 0.5 * dt;
      for (std::size_t i = 1; i < un; ++i) {
        rhs[i - 1] =
            v[i] + e * (lo[i] * v[i - 1] + mid[i] * v[i] + up[i] * v[i + 1]);
      }
    }
    rhs[0] -= f.a(0) * v0;
    rhs[interior - 1] -= f.c(interior - 1) * vn;

    if (!american) {
      f.Solve(rhs);
      std::copy(rhs.begin(), rhs.end(), v.begin() + 1);
    } else if (settings.american_solver == AmericanSolver::kBrennanSchwartz) {
      f.SolveWithFloor(rhs, std::span<const double>(payoff).subspan(1, interior),
                       std::span<double>(v).subspan(1, interior));
    } else {
      // Projected SOR from the previous time level's values.
      for (int iter = 0; iter < 10'000; ++iter) {
        double change = 0.0;
        for (std::size_t k = 0; k < interior; ++k) {
          const std::size_t i = k + 1;
          const double left = k > 0 ? f.a(k) * v[i - 1] : 0.0;
          const double right = k + 1 < interior ? f.c(k) * v[i + 1] : 0.0;
          const double gs = (rhs[k] - left - right) / f.b(k);
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
