// Chapter 9 demo: Boost.Math pricing and implied volatility.

#include <format>
#include <iostream>

#include "ch09_boost/boost_pricing.h"

int main() {
  namespace ch09 = cpp4q::hanson::ch09;
  const ch09::BlackScholesInputs in{.spot = 100.0,
                                    .strike = 110.0,
                                    .rate = 0.03,
                                    .dividend_yield = 0.01,
                                    .volatility = 0.27,
                                    .time_to_expiry = 0.75};
  const double price = ch09::BlackScholesBoost(ch09::OptionType::kCall, in);
  const auto toms = ch09::ImpliedVolToms748(ch09::OptionType::kCall, in, price);
  const auto newton =
      ch09::ImpliedVolNewton(ch09::OptionType::kCall, in, price);

  std::cout << std::format("price (boost cdf)  {:.10f}\n", price)
            << std::format("price (quadrature) {:.10f}\n",
                           ch09::PriceByQuadrature(ch09::OptionType::kCall, in))
            << std::format("implied vol TOMS748 {:.10f} in {} iters\n",
                           toms->volatility, toms->iterations)
            << std::format("implied vol Newton  {:.10f} in {} iters\n",
                           newton->volatility, newton->iterations)
            << std::format("99% normal quantile {:.6f}\n",
                           ch09::NormalQuantile(0.99));
}
