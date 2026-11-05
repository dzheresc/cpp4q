#include <gtest/gtest.h>

#include <random>
#include <stdexcept>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch11_patterns_revisited/patterns.h"

namespace cpp4q::joshi::ch11 {
namespace {

TEST(AdapterTest, StdEnginesBehindRandomBase) {
  RandomStdAdapter<std::mt19937_64> a(2, 5);
  std::vector<double> u(2), v(2), w(2);
  a.GetUniforms(u);
  a.Skip(3);
  a.GetUniforms(v);
  a.Reset();
  for (int i = 0; i < 5; ++i) a.GetUniforms(w);
  EXPECT_EQ(v, w);
  a.Reset();
  a.GetUniforms(w);
  EXPECT_EQ(u, w);
  // The 32-bit engine path gives uniforms in (0, 1) too.
  RandomStdAdapter<std::minstd_rand> small(1000, 3);
  std::vector<double> many(1000);
  small.GetUniforms(many);
  for (const double x : many) {
    EXPECT_GT(x, 0.0);
    EXPECT_LT(x, 1.0);
  }
}

TEST(AdapterTest, PricesThroughChapter6Routine) {
  const ch04::VanillaOption call(ch04::PayOffCall(100), 1.0);
  RandomStdAdapter<std::mt19937_64> gen(1, 11);
  ch05::StatisticsMoments stats;
  ch06::SimpleMonteCarlo6(call, 100, 0.2, 0.05, 100'000, stats, gen);
  const auto row = stats.GetResultsSoFar()[0];
  EXPECT_NEAR(row[0], BlackScholesCall(100, 100, 0.05, 0, 0.2, 1), 4 * row[1]);
}

TEST(CompositeTest, ButterflyPayOffAndPrice) {
  const PayOffPortfolio fly = MakeButterfly(100, 10);
  EXPECT_EQ(fly.size(), 3u);
  EXPECT_EQ(fly(85), 0);
  EXPECT_EQ(fly(95), 5);
  EXPECT_EQ(fly(100), 10);
  EXPECT_EQ(fly(108), 2);
  EXPECT_EQ(fly(130), 0);
  // Composites nest, and copy deeply through the bridge.
  PayOffPortfolio two_flies;
  two_flies.Add(1.0, fly).Add(1.0, MakeButterfly(140, 10));
  const PayOffPortfolio copy = two_flies;
  EXPECT_EQ(copy(140), 10);
  EXPECT_EQ(copy(100), 10);
  // Priced in one simulation, against the sum of closed forms.
  const ch04::VanillaOption option(fly, 1.0);
  RandomStdAdapter<std::mt19937_64> gen(1, 1);
  ch05::StatisticsMoments stats;
  ch06::SimpleMonteCarlo6(option, 100, 0.2, 0.05, 200'000, stats, gen);
  const auto c = [](double k) {
    return BlackScholesCall(100, k, 0.05, 0, 0.2, 1);
  };
  const auto row = stats.GetResultsSoFar()[0];
  EXPECT_NEAR(row[0], c(90) - 2 * c(100) + c(110), 4 * row[1]);
}

TEST(DecoratorTest, Collar) {
  const PayOffCollar collar(ch04::PayOffCall(100), 1, 20);
  EXPECT_EQ(collar(50), 1);
  EXPECT_EQ(collar(110), 10);
  EXPECT_EQ(collar(200), 20);
  EXPECT_THROW(PayOffCollar(ch04::PayOffCall(100), 5, 1),
               std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::joshi::ch11
