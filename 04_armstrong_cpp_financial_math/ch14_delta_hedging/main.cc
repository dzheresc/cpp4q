// Chapter 14 demo: hedging error against rebalancing frequency, and the
// effect of hedging at the wrong volatility.

#include <cstdio>
#include <initializer_list>

#include "ch05_multiple_files/vectors.h"
#include "ch14_delta_hedging/hedging.h"

namespace a = cpp4q::armstrong;

int main() {
  const a::ch14::BlackScholesModel world;  // drift 10%, vol 20%
  const a::ch14::EuropeanCall call(100, 1);
  a::ch14::Rng rng(14);
  std::printf("%8s %12s %12s\n", "rebal.", "mean P&L", "std P&L");
  for (int n : {4, 16, 64, 256}) {
    const a::ch14::HedgingSimulator sim(world, world, call, n);
    const auto pnl = sim.RunSimulations(10'000, rng);
    std::printf("%8d %12.5f %12.5f\n", n, a::ch05::Mean(pnl),
                a::ch05::StandardDeviation(pnl));
  }
  std::printf("\nhedging a short call with the wrong vol (true 20%%, "
              "daily rebalancing):\n");
  for (double vol : {0.15, 0.20, 0.25}) {
    a::ch14::BlackScholesModel belief = world;
    belief.volatility = vol;
    const a::ch14::HedgingSimulator sim(world, belief, call, 252);
    const auto pnl = sim.RunSimulations(5'000, rng);
    std::printf("  hedge vol %.0f%%: mean P&L %8.4f\n", 100 * vol,
                a::ch05::Mean(pnl));
  }
  return 0;
}
