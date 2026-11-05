// Chapter 1: A simple Monte Carlo model.
//
// One function prices a European call under Black-Scholes by simulating the
// terminal spot S(T) = S0 exp((r - sigma^2/2) T + sigma sqrt(T) Z) directly,
// with no time stepping. The book draws Z from rand(); here the generator is
// an explicit std::mt19937_64, so runs are reproducible and independent of
// global state, and three ways of making a Gaussian from uniforms are kept
// for comparison: summing twelve uniforms (wrong in the tails), Box-Muller
// (two logs/trig per pair) and Marsaglia's polar rejection method.

#ifndef CPP4Q_JOSHI_CH01_SIMPLE_MONTE_CARLO_SIMPLE_MC_H_
#define CPP4Q_JOSHI_CH01_SIMPLE_MONTE_CARLO_SIMPLE_MC_H_

#include <cstdint>
#include <random>

namespace cpp4q::joshi {

using Rng = std::mt19937_64;

// A Monte Carlo estimate and its standard error.
struct McResult {
  double price;
  double std_error;
};

// Uniform on the open interval (0, 1): 53 random bits, offset by half an ulp.
inline double UniformOpen(Rng& rng) {
  return (static_cast<double>(rng() >> 11) + 0.5) * 0x1.0p-53;
}

}  // namespace cpp4q::joshi

namespace cpp4q::joshi::ch01 {

// Sum of 12 uniforms minus 6: mean 0, variance 1, support [-6, 6].
double GetOneGaussianBySummation(Rng& rng);
// Box-Muller: sqrt(-2 log u1) cos(2 pi u2). Discards the sine partner.
double GetOneGaussianByBoxMuller(Rng& rng);
// Marsaglia's polar method. Discards the second variate of the pair.
double GetOneGaussianByPolar(Rng& rng);

// Price of a European call with `num_paths` terminal-value samples.
McResult SimpleMonteCarlo1(double expiry, double strike, double spot,
                           double vol, double r, std::uint64_t num_paths,
                           Rng& rng);

}  // namespace cpp4q::joshi::ch01

#endif  // CPP4Q_JOSHI_CH01_SIMPLE_MONTE_CARLO_SIMPLE_MC_H_
