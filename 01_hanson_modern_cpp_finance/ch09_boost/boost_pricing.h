// Chapter 9: The Boost Libraries.
//
// Boost.Math (header-only) for distributions, root finding and quadrature:
//  * boost::math::normal for the CDF / quantile,
//  * tools::newton_raphson_iterate and tools::toms748_solve for implied vol,
//  * quadrature::gauss_kronrod to price by integrating payoff x density.

#ifndef CPP4Q_HANSON_CH09_BOOST_BOOST_PRICING_H_
#define CPP4Q_HANSON_CH09_BOOST_BOOST_PRICING_H_

#include <cstdint>
#include <optional>

#include "ch01_overview/black_scholes.h"

namespace cpp4q::hanson::ch09 {

using ch01::BlackScholesInputs;
using ch01::OptionType;

// Black-Scholes using boost::math::cdf(normal, x).
double BlackScholesBoost(OptionType type, const BlackScholesInputs& in);

struct ImpliedVolResult {
  double volatility;
  std::uintmax_t iterations;
};

// Implied vol by bracketing TOMS 748 on [1e-6, 5]. std::nullopt if the
// price is outside the no-arbitrage bounds. `in.volatility` is ignored.
std::optional<ImpliedVolResult> ImpliedVolToms748(OptionType type,
                                                  const BlackScholesInputs& in,
                                                  double price);

// Implied vol by Newton-Raphson using vega, started from the
// Brenner-Subrahmanyam guess.
std::optional<ImpliedVolResult> ImpliedVolNewton(OptionType type,
                                                 const BlackScholesInputs& in,
                                                 double price);

// Discounted E[payoff(S_T)] under the lognormal density, integrated with
// 61-point Gauss-Kronrod over log-spot. Converges to the closed form.
double PriceByQuadrature(OptionType type, const BlackScholesInputs& in);

// Normal quantile (inverse CDF), e.g. for parametric VaR.
double NormalQuantile(double p);

}  // namespace cpp4q::hanson::ch09

#endif  // CPP4Q_HANSON_CH09_BOOST_BOOST_PRICING_H_
