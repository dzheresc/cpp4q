// Chapter 7: Estimating sensitivities.
//
// Three ways to get a Greek from simulation, on contracts whose Greeks are
// known in closed form:
//   finite differences (7.1) - bump and reprice; common random numbers
//                              make the two prices move together, which
//                              turns an O(1/h^2) variance into O(1);
//   pathwise (7.2)           - differentiate the payoff along the path;
//                              unbiased when the payoff is Lipschitz
//                              (calls, Asians), useless for digitals;
//   likelihood ratio (7.3)   - differentiate the density instead, so any
//                              payoff works, at the price of more variance.

#ifndef CPP4Q_GLASSERMAN_CH07_SENSITIVITIES_GREEKS_H_
#define CPP4Q_GLASSERMAN_CH07_SENSITIVITIES_GREEKS_H_

#include <cstddef>
#include <cstdint>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch07 {

using ch01::BlackScholesInputs;
using ch01::Estimate;

enum class FiniteDifference { kForward, kCentral };

// Call delta by bumping the spot by h (relative: h * spot).
Estimate FiniteDifferenceDelta(const BlackScholesInputs& in, double rel_bump,
                               FiniteDifference kind,
                               bool common_random_numbers, std::size_t paths,
                               std::uint64_t seed);

// All from one set of draws of S(T).
struct CallGreeks {
  Estimate price;
  Estimate pathwise_delta;
  Estimate pathwise_vega;
  Estimate lr_delta;
  Estimate lr_vega;
  Estimate lr_gamma;
};
CallGreeks EuropeanCallGreeks(const BlackScholesInputs& in, std::size_t paths,
                              std::uint64_t seed);

// Digital call (pays 1 if S(T) > K): the pathwise derivative is zero almost
// surely, so only the likelihood ratio estimator is valid.
struct DigitalGreeks {
  Estimate price;
  Estimate pathwise_delta;  // always 0: biased
  Estimate lr_delta;
};
DigitalGreeks DigitalCallGreeks(const BlackScholesInputs& in, std::size_t paths,
                                std::uint64_t seed);

// Arithmetic Asian call delta: pathwise (Abar / S0 on exercise) and
// likelihood ratio (score Z_1 / (S0 vol sqrt(t_1)) through the first step).
struct AsianDelta {
  Estimate price;
  Estimate pathwise_delta;
  Estimate lr_delta;
};
AsianDelta AsianCallDelta(const BlackScholesInputs& in, int fixings,
                          std::size_t paths, std::uint64_t seed);

}  // namespace cpp4q::glasserman::ch07

#endif  // CPP4Q_GLASSERMAN_CH07_SENSITIVITIES_GREEKS_H_
