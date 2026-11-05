// Chapter 1: Foundations.
//
// The basic Monte Carlo estimator: draw S(T) exactly from its lognormal
// law, average the discounted payoff, report the standard error. The
// chapter's points are that the error shrinks like n^(-1/2) regardless of
// dimension, and that estimators should be compared by variance times
// work, not variance alone. This chapter uses only <random>; chapter 2
// replaces it with generators whose behaviour we control.

#ifndef CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_MONTE_CARLO_H_
#define CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_MONTE_CARLO_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch01 {

struct CallPayoff {
  double strike;
  double operator()(double s) const { return std::max(s - strike, 0.0); }
};

struct PutPayoff {
  double strike;
  double operator()(double s) const { return std::max(strike - s, 0.0); }
};

// Any payoff of S(T) under Black-Scholes dynamics.
template <typename Payoff>
Estimate PriceEuropean(const BlackScholesInputs& in, Payoff payoff,
                       std::size_t paths, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::normal_distribution<double> normal;
  const double t = in.maturity;
  const double drift = (in.rate - in.dividend - 0.5 * in.vol * in.vol) * t;
  const double sd = in.vol * std::sqrt(t);
  const double df = std::exp(-in.rate * t);
  RunningStats stats;
  for (std::size_t i = 0; i < paths; ++i) {
    const double s_t = in.spot * std::exp(drift + sd * normal(rng));
    stats.Add(df * payoff(s_t));
  }
  return stats.ToEstimate();
}

Estimate PriceEuropeanCall(const BlackScholesInputs& in, std::size_t paths,
                           std::uint64_t seed);

// Arithmetic-average Asian call with fixings at i T / n, i = 1..n. No
// closed form: the simplest case where simulation is the method of choice.
Estimate PriceArithmeticAsianCall(const BlackScholesInputs& in, int fixings,
                                  std::size_t paths, std::uint64_t seed);

struct ConvergencePoint {
  std::size_t paths;
  double abs_error;
  double std_error;
};

// Prices a call with 2^min_log2 .. 2^max_log2 paths (independent runs) and
// records the error against Black-Scholes.
std::vector<ConvergencePoint> ConvergenceStudy(const BlackScholesInputs& in,
                                               int min_log2, int max_log2,
                                               std::uint64_t seed);

// Least-squares slope of log(std_error) against log(paths); ~ -0.5.
double StdErrorSlope(const std::vector<ConvergencePoint>& points);

// Glasserman 1.1.3: with a budget of t seconds and tau seconds per sample,
// the error is ~ sqrt(variance * tau / t). Lower variance * tau wins.
inline double WorkNormalizedVariance(double variance,
                                     double seconds_per_sample) {
  return variance * seconds_per_sample;
}

}  // namespace cpp4q::glasserman::ch01

#endif  // CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_MONTE_CARLO_H_
