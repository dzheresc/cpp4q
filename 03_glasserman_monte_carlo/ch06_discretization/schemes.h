// Chapter 6: Discretization methods.
//
// Euler and Milstein steps for a scalar SDE dX = a(t, X) dt + b(t, X) dW,
// and the experiments that measure them on GBM, where the exact solution
// driven by the same Brownian path is known:
//   strong order (6.1.1) - E|X^h(T) - X(T)| ~ h^(1/2) for Euler, h for
//                          Milstein;
//   weak order (6.1.2)   - |E f(X^h(T)) - E f(X(T))| ~ h for Euler, which
//                          Richardson extrapolation 2 V(h/2) - V(h) cuts
//                          to O(h^2) (6.2.4).
// For CIR, Euler can step below zero where sqrt(r) is undefined; three
// standard fixes are compared against the exact bond price.

#ifndef CPP4Q_GLASSERMAN_CH06_DISCRETIZATION_SCHEMES_H_
#define CPP4Q_GLASSERMAN_CH06_DISCRETIZATION_SCHEMES_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"
#include "ch03_sample_paths/gbm.h"
#include "ch03_sample_paths/short_rate.h"

namespace cpp4q::glasserman::ch06 {

template <typename Drift, typename Diffusion>
double EulerStep(const Drift& a, const Diffusion& b, double t, double x,
                 double dt, double dw) {
  return x + a(t, x) * dt + b(t, x) * dw;
}

// Adds the Ito correction 1/2 b b' (dW^2 - dt), b' = db/dx.
template <typename Drift, typename Diffusion, typename DiffusionDx>
double MilsteinStep(const Drift& a, const Diffusion& b, const DiffusionDx& db,
                    double t, double x, double dt, double dw) {
  const double bx = b(t, x);
  return x + a(t, x) * dt + bx * dw + 0.5 * bx * db(t, x) * (dw * dw - dt);
}

enum class Scheme { kEuler, kMilstein };

// Mean absolute error at T between the scheme and the exact GBM solution
// on the same Brownian path.
ch01::Estimate GbmStrongError(const ch03::GbmParams& p, double maturity,
                              std::size_t steps, Scheme scheme,
                              std::size_t paths, std::uint64_t seed);

// Least-squares slope of log(error) against log(h = T / steps).
double FitOrder(std::span<const std::size_t> steps,
                std::span<const double> errors);

// Exact moments of the Euler approximation of GBM: E[X^h(T)] and
// E[X^h(T)^2] (6.1.2), to show weak order 1 without sampling noise.
double EulerGbmMean(const ch03::GbmParams& p, double maturity,
                    std::size_t steps);
double EulerGbmSecondMoment(const ch03::GbmParams& p, double maturity,
                            std::size_t steps);

// Euler call price; two_point replaces sqrt(h) Z by +-sqrt(h) with
// probability 1/2 each (same weak order, cheaper).
ch01::Estimate EulerCallPrice(const ch01::BlackScholesInputs& in,
                              std::size_t steps, std::size_t paths,
                              bool two_point, std::uint64_t seed);

// 2 V(h/2) - V(h) with coarse increments summed from the fine ones, so
// the two levels share their randomness.
ch01::Estimate RichardsonCallPrice(const ch01::BlackScholesInputs& in,
                                   std::size_t coarse_steps, std::size_t paths,
                                   std::uint64_t seed);

enum class CirFix { kAbsorb, kReflect, kFullTruncation };

ch01::Estimate CirEulerBond(const ch03::ShortRateParams& p, double r0,
                            double maturity, std::size_t steps, CirFix fix,
                            std::size_t paths, std::uint64_t seed);

}  // namespace cpp4q::glasserman::ch06

#endif  // CPP4Q_GLASSERMAN_CH06_DISCRETIZATION_SCHEMES_H_
