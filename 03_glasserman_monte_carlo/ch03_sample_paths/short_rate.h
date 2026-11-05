// Chapter 3: Generating sample paths.
//
// Short-rate models with exact transition laws (3.3, 3.4).
// Vasicek: dr = kappa (theta - r) dt + sigma dW. The pair (r(t+h),
// integral of r over [t, t+h]) is bivariate normal, so the discount factor
// exp(-integral r) can be sampled with no discretization bias at all.
// CIR: dr = kappa (theta - r) dt + sigma sqrt(r) dW. r(t+h) given r(t) is
// a scaled noncentral chi-square, sampled exactly with chapter 2's mixture.

#ifndef CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_SHORT_RATE_H_
#define CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_SHORT_RATE_H_

#include <cstddef>

#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch03 {

struct ShortRateParams {
  double kappa = 0.5;   // mean-reversion speed
  double theta = 0.04;  // long-run level
  double sigma = 0.01;
};

double VasicekBondPrice(const ShortRateParams& p, double r0, double t);

struct RateAndIntegral {
  double rate;
  double integral;  // of r over the step
};

class VasicekExactStep {
 public:
  VasicekExactStep(const ShortRateParams& p, double dt);
  // Two independent standard normals give one exact transition.
  RateAndIntegral Step(double r, double z1, double z2) const;

 private:
  ShortRateParams p_;
  double decay_;     // exp(-kappa dt)
  double int_coef_;  // (1 - decay) / kappa
  double dt_;
  double sd_r_;
  double sd_y_cond_;  // sd of the integral given r(t+h)
  double beta_;       // regression of the integral on the r(t+h) shock
};

// E[exp(-integral_0^T r)] by exact simulation.
ch01::Estimate VasicekBondMc(const ShortRateParams& p, double r0,
                             double maturity, std::size_t steps,
                             std::size_t paths, ch02::Sampler& s);

double CirBondPrice(const ShortRateParams& p, double r0, double t);
double CirExactStep(const ShortRateParams& p, double r, double dt,
                    ch02::Sampler& s);
// Exact CIR rates on the grid, trapezoid rule for the integral (the only
// source of bias, O(dt^2)).
ch01::Estimate CirBondMc(const ShortRateParams& p, double r0, double maturity,
                         std::size_t steps, std::size_t paths,
                         ch02::Sampler& s);

}  // namespace cpp4q::glasserman::ch03

#endif  // CPP4Q_GLASSERMAN_CH03_SAMPLE_PATHS_SHORT_RATE_H_
