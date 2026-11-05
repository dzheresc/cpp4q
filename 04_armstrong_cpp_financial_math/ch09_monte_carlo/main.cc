// Chapter 9 demo: Monte Carlo prices converging to Black-Scholes, and a
// few simulated paths in both measures.

#include <cstdio>
#include <initializer_list>

#include "ch09_monte_carlo/monte_carlo.h"

namespace a = cpp4q::armstrong;

int main() {
  const a::ch08::BlackScholesModel model;
  const a::ch08::CallOption call(100, 1);
  const double exact = call.Price(model);
  a::ch09::Rng rng(9);
  std::printf("Black-Scholes call: %.5f\n%10s %10s %10s\n", exact, "paths",
              "MC", "std err");
  for (int n : {1'000, 10'000, 100'000, 1'000'000}) {
    const auto e = a::ch09::MonteCarloPricer(n).Price(call, model, rng);
    std::printf("%10d %10.5f %10.5f\n", n, e.price, e.std_error);
  }
  std::printf("\nfirst 5 monthly prices, real world vs risk neutral:\n");
  const auto real = a::ch09::GeneratePricePath(model, 5.0 / 12, 5,
                                               a::ch09::Measure::kRealWorld,
                                               rng);
  const auto rn = a::ch09::GeneratePricePath(model, 5.0 / 12, 5,
                                             a::ch09::Measure::kRiskNeutral,
                                             rng);
  for (std::size_t i = 0; i < real.size(); ++i) {
    std::printf("  month %zu: %8.3f %8.3f\n", i + 1, real[i], rn[i]);
  }
  return 0;
}
