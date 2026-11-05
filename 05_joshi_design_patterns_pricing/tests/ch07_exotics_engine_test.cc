#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch07_exotics_engine/exotic_engine.h"

namespace cpp4q::joshi::ch07 {
namespace {

std::vector<double> Monthly(int n, double expiry) {
  std::vector<double> t;
  for (int i = 1; i <= n; ++i) t.push_back(expiry * i / n);
  return t;
}

std::vector<double> Simulate(const PathDependent& product, double spot,
                             double r, double d, double vol,
                             std::uint64_t paths, std::uint64_t seed) {
  ExoticBsEngine engine(product, r, d, vol,
                        ch06::RandomParkMiller(1, seed), spot);
  ch05::StatisticsMoments stats;
  engine.DoSimulation(stats, paths);
  return stats.GetResultsSoFar()[0];
}

TEST(PathDependentTest, CashFlowsOfOnePath) {
  const std::vector<double> path{90, 110, 130};
  std::vector<CashFlow> flows(1);
  const PathDependentAsian asian({1, 2, 3}, 3.5, ch04::PayOffCall(100));
  ASSERT_EQ(asian.CashFlows(path, flows), 1u);
  EXPECT_DOUBLE_EQ(flows[0].amount, 10);
  EXPECT_EQ(asian.PossibleCashFlowTimes(), std::vector<double>{3.5});
  const PathDependentGeometricAsian geo({1, 2, 3}, 3, ch04::PayOffCall(100));
  geo.CashFlows(path, flows);
  EXPECT_NEAR(flows[0].amount, std::cbrt(90.0 * 110 * 130) - 100, 1e-12);
  const PathDependentKnockOut up({1, 2, 3}, 120, BarrierDirection::kUp,
                                 ch04::PayOffCall(100), 2.5);
  up.CashFlows(path, flows);
  EXPECT_EQ(flows[0].amount, 2.5);
  EXPECT_EQ(flows[0].time_index, 2u);
  const PathDependentKnockOut down({1, 2, 3}, 80, BarrierDirection::kDown,
                                   ch04::PayOffCall(100));
  down.CashFlows(path, flows);
  EXPECT_EQ(flows[0].amount, 30);
  EXPECT_THROW(PathDependentAsian({1, 1}, 1, ch04::PayOffCall(1)),
               std::invalid_argument);
}

TEST(ExoticEngineTest, SingleFixingAsianIsAEuropean) {
  const PathDependentAsian asian({1.0}, 1.0, ch04::PayOffCall(100));
  const auto row = Simulate(asian, 100, 0.05, 0.02, 0.2, 100'000, 1);
  EXPECT_NEAR(row[0], BlackScholesCall(100, 100, 0.05, 0.02, 0.2, 1),
              4 * row[1]);
}

TEST(ExoticEngineTest, GeometricAsianMatchesClosedForm) {
  const auto times = Monthly(12, 1.0);
  const PathDependentGeometricAsian geo(times, 1.0, ch04::PayOffCall(95));
  const auto row = Simulate(geo, 100, 0.05, 0.01, 0.3, 100'000, 2);
  EXPECT_NEAR(row[0], GeometricAsianCall(100, 95, 0.05, 0.01, 0.3, times, 1.0),
              4 * row[1]);
  // With one fixing the closed form is Black-Scholes.
  const std::vector<double> one{2.0};
  EXPECT_NEAR(GeometricAsianCall(100, 95, 0.05, 0.01, 0.3, one, 2.0),
              BlackScholesCall(100, 95, 0.05, 0.01, 0.3, 2.0), 1e-12);
}

TEST(ExoticEngineTest, ArithmeticAsianAboveGeometric) {
  const auto times = Monthly(12, 1.0);
  const PathDependentAsian arith(times, 1.0, ch04::PayOffCall(100));
  const auto row = Simulate(arith, 100, 0.05, 0, 0.2, 50'000, 3);
  const double geo = GeometricAsianCall(100, 100, 0.05, 0, 0.2, times, 1.0);
  EXPECT_GT(row[0], geo);
  EXPECT_LT(row[0], geo + 0.5);  // the AM-GM gap is small at 20% vol
}

TEST(ExoticEngineTest, DiscreteKnockOutNearShiftedContinuous) {
  // Broadie-Glasserman-Kou: m discrete fixings ~ a continuous barrier
  // shifted away from the spot by exp(-0.5826 vol sqrt(T / m)).
  const int m = 50;
  const double h = 90, vol = 0.25, t = 1;
  const PathDependentKnockOut ko(Monthly(m, t), h, BarrierDirection::kDown,
                                 ch04::PayOffCall(100));
  const auto row = Simulate(ko, 100, 0.05, 0, vol, 100'000, 4);
  const double shifted = h * std::exp(-0.5826 * vol * std::sqrt(t / m));
  EXPECT_NEAR(row[0],
              DownAndOutCallContinuous(100, 100, shifted, 0.05, 0, vol, t),
              4 * row[1] + 0.02);
  EXPECT_LT(row[0], BlackScholesCall(100, 100, 0.05, 0, vol, t));
}

TEST(ExoticEngineTest, RebateDiscountedFromKnockTime) {
  // Spot starts below a down barrier: knocked at the first fixing.
  const PathDependentKnockOut ko({0.5, 1.0}, 200, BarrierDirection::kDown,
                                 ch04::PayOffCall(100), 3.0);
  const auto row = Simulate(ko, 100, 0.06, 0, 0.2, 100, 5);
  EXPECT_NEAR(row[0], 3.0 * std::exp(-0.06 * 0.5), 1e-12);
  EXPECT_EQ(row[2], 0.0);
}

TEST(ExoticEngineTest, TimeDependentRatesDiscountCorrectly) {
  // A zero-vol single fixing: the price is exactly discounted intrinsic.
  const ch04::Parameters r = ch04::ParametersPiecewiseConstant({0.5}, {0.02, 0.08});
  const PathDependentAsian fwd({1.0}, 1.0, ch04::PayOffCall(50));
  ExoticBsEngine engine(fwd, r, 0.0, 0.0, ch06::RandomParkMiller(1), 100);
  ch05::StatisticsMean mean;
  engine.DoSimulation(mean, 10);
  const double df = std::exp(-0.05);
  EXPECT_NEAR(mean.GetResultsSoFar()[0][0], (100 / df - 50) * df, 1e-10);
}

}  // namespace
}  // namespace cpp4q::joshi::ch07
