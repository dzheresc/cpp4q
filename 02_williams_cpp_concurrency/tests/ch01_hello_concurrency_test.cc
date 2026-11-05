#include <gtest/gtest.h>

#include <cmath>

#include "ch01_hello_concurrency/swap_pricing.h"

namespace cpp4q::williams::ch01 {
namespace {

TEST(SwapPricingTest, ConcurrentMatchesSequential) {
  const Leg fixed = MakeFixedLeg(1e6, 0.04, 10);
  const Leg floating = MakeFixedLeg(1e6, 0.03, 10);
  const auto v = PriceSwapConcurrently(fixed, floating, 0.035);
  EXPECT_DOUBLE_EQ(v.fixed_leg_pv, LegPv(fixed, 0.035));
  EXPECT_DOUBLE_EQ(v.floating_leg_pv, LegPv(floating, 0.035));
  EXPECT_GT(v.Npv(), 0.0);  // receive the higher coupon
}

TEST(SwapPricingTest, LegPvOfZeroCouponIsDiscountFactor) {
  const Leg zcb{{.time = 2.0, .amount = 100.0}};
  EXPECT_NEAR(LegPv(zcb, 0.05), 100.0 * std::exp(-0.1), 1e-12);
}

TEST(SwapPricingTest, FixedLegShape) {
  const Leg leg = MakeFixedLeg(100.0, 0.08, 2);
  ASSERT_EQ(leg.size(), 9u);  // 8 coupons + principal
  EXPECT_DOUBLE_EQ(leg.front().amount, 2.0);
  EXPECT_DOUBLE_EQ(leg.back().amount, 100.0);
  EXPECT_DOUBLE_EQ(leg.back().time, 2.0);
}

TEST(SwapPricingTest, HardwareThreadsAtLeastOne) {
  EXPECT_GE(HardwareThreads(), 1u);
}

}  // namespace
}  // namespace cpp4q::williams::ch01
