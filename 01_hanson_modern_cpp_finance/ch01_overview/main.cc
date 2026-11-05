// Chapter 1 demo: price a call and put, check put-call parity.

#include <cmath>
#include <format>
#include <iostream>

#include "ch01_overview/black_scholes.h"

int main() {
  using cpp4q::hanson::ch01::BlackScholesInputs;
  using cpp4q::hanson::ch01::OptionType;
  namespace ch01 = cpp4q::hanson::ch01;

  // Evaluated at compile time.
  static_assert(ch01::Payoff(OptionType::kCall, 110.0, 100.0) == 10.0);
  constexpr double kMonthlyGrowth = ch01::CompoundGrowth(0.12, 12, 1);

  const BlackScholesInputs in{.spot = 100.0,
                              .strike = 105.0,
                              .rate = 0.04,
                              .dividend_yield = 0.01,
                              .volatility = 0.2,
                              .time_to_expiry = 0.5};
  const double call = ch01::BlackScholesPrice(OptionType::kCall, in);
  const double put = ch01::BlackScholesPrice(OptionType::kPut, in);
  const double parity =
      in.spot * std::exp(-in.dividend_yield * in.time_to_expiry) -
      in.strike * std::exp(-in.rate * in.time_to_expiry);
  const auto g = ch01::BlackScholesGreeks(OptionType::kCall, in);

  std::cout << std::format("12% compounded monthly for 1y: {:.6f}\n",
                           kMonthlyGrowth)
            << std::format("call = {:.4f}  put = {:.4f}\n", call, put)
            << std::format("C - P = {:.6f}  S e^-qT - K e^-rT = {:.6f}\n",
                           call - put, parity)
            << std::format(
                   "delta {:.4f} gamma {:.4f} vega {:.4f} theta {:.4f}"
                   " rho {:.4f}\n",
                   g.delta, g.gamma, g.vega, g.theta, g.rho);
}
