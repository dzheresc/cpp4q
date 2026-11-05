// Chapter 5: Quasi-Monte Carlo.
//
// Pricing an arithmetic Asian option with Sobol points. Each point in
// [0,1)^d becomes d normals by inversion and then a Brownian path. With
// the random walk every dimension matters equally; with the Brownian
// bridge the first coordinates carry most of the variance, and those are
// the coordinates where Sobol points are most uniform (5.5.1: effective
// dimension). Randomized QMC repeats the rule under independent digital
// shifts to get an unbiased estimate and a standard error.

#ifndef CPP4Q_GLASSERMAN_CH05_QUASI_MONTE_CARLO_QMC_PRICING_H_
#define CPP4Q_GLASSERMAN_CH05_QUASI_MONTE_CARLO_QMC_PRICING_H_

#include <cstddef>
#include <cstdint>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch05 {

enum class PathConstruction { kRandomWalk, kBrownianBridge };

struct QmcAsianSpec {
  ch01::BlackScholesInputs market;
  int fixings = 16;  // also the Sobol dimension, at most 16
};

// One QMC estimate with `points` points; shift_seed 0 means no shift.
double QmcAsianCall(const QmcAsianSpec& spec, std::size_t points,
                    PathConstruction construction,
                    std::uint64_t shift_seed = 0);

// Mean and standard error over `replications` independent random shifts.
ch01::Estimate RqmcAsianCall(const QmcAsianSpec& spec, std::size_t points,
                             std::size_t replications,
                             PathConstruction construction, std::uint64_t seed);

// Same budget with pseudo-random Mrg32k3a normals, for comparison.
ch01::Estimate McAsianCall(const QmcAsianSpec& spec, std::size_t paths,
                           std::uint64_t seed);

}  // namespace cpp4q::glasserman::ch05

#endif  // CPP4Q_GLASSERMAN_CH05_QUASI_MONTE_CARLO_QMC_PRICING_H_
