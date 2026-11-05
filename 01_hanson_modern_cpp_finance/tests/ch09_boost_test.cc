#include <gtest/gtest.h>

#include <cmath>

#include "ch09_boost/boost_pricing.h"

namespace cpp4q::hanson::ch09 {
namespace {

TEST(BoostPricingTest, MatchesChapterOne) {
  for (double k : {60.0, 90.0, 100.0, 115.0, 180.0}) {
    const BlackScholesInputs in{100.0, k, 0.04, 0.015, 0.3, 1.25};
    for (auto type : {OptionType::kCall, OptionType::kPut}) {
      const double ref = ch01::BlackScholesPrice(type, in);
      EXPECT_NEAR(BlackScholesBoost(type, in), ref, 1e-11);
      EXPECT_NEAR(PriceByQuadrature(type, in), ref, 1e-9);
    }
  }
}

TEST(ImpliedVolTest, RecoversVolatility) {
  for (double vol : {0.05, 0.2, 0.6, 1.5}) {
    for (double k : {70.0, 100.0, 140.0}) {
      const BlackScholesInputs in{100.0, k, 0.02, 0.0, vol, 0.5};
      for (auto type : {OptionType::kCall, OptionType::kPut}) {
        const double px = ch01::BlackScholesPrice(type, in);
        // Strip the forward intrinsic value: with no time value left, vega
        // is ~0 and the volatility is not identifiable from the price.
        const double fwd_intrinsic =
            std::exp(-in.rate * in.time_to_expiry) *
            ch01::Payoff(type, in.spot * std::exp(in.rate * in.time_to_expiry),
                         in.strike);
        if (px - fwd_intrinsic < 1e-8) continue;
        const auto toms = ImpliedVolToms748(type, in, px);
        const auto newton = ImpliedVolNewton(type, in, px);
        ASSERT_TRUE(toms && newton);
        EXPECT_NEAR(toms->volatility, vol, 1e-7) << k << ' ' << vol;
        EXPECT_NEAR(newton->volatility, vol, 1e-7) << k << ' ' << vol;
      }
    }
  }
}

TEST(ImpliedVolTest, RejectsArbitragePrices) {
  const BlackScholesInputs in{100.0, 100.0, 0.0, 0.0, 0.2, 1.0};
  EXPECT_FALSE(ImpliedVolToms748(OptionType::kCall, in, 150.0));
  EXPECT_FALSE(ImpliedVolNewton(OptionType::kCall, in, -1.0));
}

TEST(NormalQuantileTest, Inverts) {
  EXPECT_NEAR(NormalQuantile(0.975), 1.959963984540054, 1e-14);
  EXPECT_NEAR(ch01::NormCdf(NormalQuantile(0.3)), 0.3, 1e-15);
}

}  // namespace
}  // namespace cpp4q::hanson::ch09
