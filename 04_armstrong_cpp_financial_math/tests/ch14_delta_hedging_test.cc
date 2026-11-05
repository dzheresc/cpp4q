#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "ch05_multiple_files/vectors.h"
#include "ch14_delta_hedging/hedging.h"

namespace cpp4q::armstrong::ch14 {
namespace {

TEST(HedgingTest, MeanZeroAndErrorFallsLikeRootN) {
  const BlackScholesModel world;  // drift 10% != r: irrelevant once hedged
  const EuropeanCall call(100, 1);
  Rng rng(14);
  constexpr int kPaths = 4000;
  const auto coarse = HedgingSimulator(world, world, call, 16)
                          .RunSimulations(kPaths, rng);
  const auto fine = HedgingSimulator(world, world, call, 256)
                        .RunSimulations(kPaths, rng);
  const double sd_coarse = ch05::StandardDeviation(coarse);
  const double sd_fine = ch05::StandardDeviation(fine);
  EXPECT_NEAR(ch05::Mean(coarse), 0, 4 * sd_coarse / std::sqrt(kPaths));
  EXPECT_NEAR(ch05::Mean(fine), 0, 4 * sd_fine / std::sqrt(kPaths));
  // 16x the rebalancing: std falls by sqrt(16) = 4.
  EXPECT_NEAR(sd_coarse / sd_fine, 4.0, 0.4);
  // Unhedged, the short call's P&L spread is the option's own (~14).
  EXPECT_LT(sd_fine, 0.5);
}

TEST(HedgingTest, WrongVolatilityBiasesThePnl) {
  const BlackScholesModel world;
  BlackScholesModel high = world, low = world;
  high.volatility = 0.3;
  low.volatility = 0.1;
  const EuropeanCall call(100, 1);
  Rng rng(15);
  // Selling at too high a vol profits; too low loses (gamma P&L).
  const double sold_rich = ch05::Mean(
      HedgingSimulator(world, high, call, 100).RunSimulations(2000, rng));
  const double sold_cheap = ch05::Mean(
      HedgingSimulator(world, low, call, 100).RunSimulations(2000, rng));
  EXPECT_GT(sold_rich, 1.0);
  EXPECT_LT(sold_cheap, -1.0);
}

TEST(HedgingTest, OneStepIsStaticHedge) {
  const BlackScholesModel world;
  const EuropeanCall call(100, 1);
  Rng rng(16);
  const HedgingSimulator sim(world, world, call, 1);
  // With one step, P&L = (C0 - D0 S0) e^{rT} + D0 S_T - (S_T - K)^+, which
  // is a deterministic function of S_T; just check it is finite and the
  // constructor's guards work.
  EXPECT_TRUE(std::isfinite(sim.RunSimulation(rng)));
  EXPECT_THROW(HedgingSimulator(world, world, call, 0), std::invalid_argument);
  BlackScholesModel late = world;
  late.date = 2;
  EXPECT_THROW(HedgingSimulator(late, late, call, 10), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch14
