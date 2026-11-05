#include <gtest/gtest.h>

#include <cmath>
#include <functional>
#include <memory>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch12_situation_2007/modern.h"

namespace cpp4q::joshi::ch12 {
namespace {

static_assert(PayOffLike<Call>);
static_assert(PayOffLike<DoubleDigital>);
static_assert(!PayOffLike<int>);

TEST(ModernTest, AllDispatchMechanismsAgree) {
  const BsInputs in{.spot = 100, .r = 0.03, .vol = 0.3, .expiry = 1.5};
  Rng rng(12);
  const auto spots = GenerateTerminalSpots(in, 100'000, rng);
  const ch04::PayOffPut dynamic(95);
  const Put value{95};
  const double v = MeanPayOffVirtual(dynamic, spots);
  EXPECT_EQ(MeanPayOffFunction(value, spots), v);
  // Blocked summation changes the rounding, not the value.
  EXPECT_NEAR(MeanPayOffVirtualBatched(dynamic, spots), v, 1e-12 * v);
  EXPECT_EQ(MeanPayOffTemplate(value, spots), v);
  EXPECT_EQ(MeanPayOffVariant(PayOffVariant(value), spots), v);
  // And the answer is right.
  const double df = std::exp(-in.r * in.expiry);
  double sum_sq = 0;
  for (const double s : spots) sum_sq += value(s) * value(s);
  const double n = static_cast<double>(spots.size());
  const double se = df * std::sqrt((sum_sq / n - v * v) / n);
  EXPECT_NEAR(df * v, BlackScholesPut(100, 95, 0.03, 0, 0.3, 1.5), 4 * se);
}

TEST(ModernTest, VariantDispatchesEachAlternative) {
  const std::vector<double> spots{80, 100, 120};
  EXPECT_DOUBLE_EQ(MeanPayOffVariant(Call{100}, spots), 20.0 / 3);
  EXPECT_DOUBLE_EQ(MeanPayOffVariant(Put{100}, spots), 20.0 / 3);
  EXPECT_DOUBLE_EQ(MeanPayOffVariant(DoubleDigital{90, 130}, spots), 2.0 / 3);
}

TEST(ModernTest, SharedPayOffIsCountedNotCloned) {
  const auto payoff = std::make_shared<const ch04::PayOffCall>(100);
  const SharedVanillaOption a(payoff, 1);
  {
    const SharedVanillaOption b = a;
    EXPECT_EQ(a.use_count(), 3);
    EXPECT_EQ(b.OptionPayOff(110), 10);
  }
  EXPECT_EQ(a.use_count(), 2);
  EXPECT_THROW(SharedVanillaOption(nullptr, 1), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::joshi::ch12
