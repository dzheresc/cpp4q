#include <gtest/gtest.h>

#include <cmath>

#include "ch01_overview/black_scholes.h"

namespace cpp4q::hanson::ch01 {
namespace {

constexpr BlackScholesInputs kAtm{.spot = 100.0,
                                  .strike = 100.0,
                                  .rate = 0.05,
                                  .dividend_yield = 0.0,
                                  .volatility = 0.2,
                                  .time_to_expiry = 1.0};

TEST(NormCdfTest, KnownValues) {
  EXPECT_DOUBLE_EQ(NormCdf(0.0), 0.5);
  EXPECT_NEAR(NormCdf(1.96), 0.9750021048517795, 1e-15);
  EXPECT_NEAR(NormCdf(-1.0) + NormCdf(1.0), 1.0, 1e-15);
}

TEST(BlackScholesTest, ReferencePrices) {
  // Hull's textbook ATM example: C = 10.4506, P = 5.5735.
  EXPECT_NEAR(BlackScholesPrice(OptionType::kCall, kAtm), 10.450583572, 1e-8);
  EXPECT_NEAR(BlackScholesPrice(OptionType::kPut, kAtm), 5.573526022, 1e-8);
}

TEST(BlackScholesTest, PutCallParityWithDividends) {
  BlackScholesInputs in = kAtm;
  in.dividend_yield = 0.02;
  in.strike = 90.0;
  const double lhs = BlackScholesPrice(OptionType::kCall, in) -
                     BlackScholesPrice(OptionType::kPut, in);
  const double rhs =
      in.spot * std::exp(-in.dividend_yield) - in.strike * std::exp(-in.rate);
  EXPECT_NEAR(lhs, rhs, 1e-12);
}

TEST(BlackScholesTest, DegenerateInputsGiveIntrinsic) {
  BlackScholesInputs in = kAtm;
  in.time_to_expiry = 0.0;
  in.spot = 120.0;
  EXPECT_DOUBLE_EQ(BlackScholesPrice(OptionType::kCall, in), 20.0);
  EXPECT_DOUBLE_EQ(BlackScholesPrice(OptionType::kPut, in), 0.0);
}

TEST(BlackScholesTest, GreeksMatchFiniteDifferences) {
  for (const auto type : {OptionType::kCall, OptionType::kPut}) {
    const Greeks g = BlackScholesGreeks(type, kAtm);
    constexpr double h = 1e-4;
    auto bump = [&](auto member, double dx) {
      BlackScholesInputs up = kAtm;
      BlackScholesInputs dn = kAtm;
      up.*member += dx;
      dn.*member -= dx;
      return (BlackScholesPrice(type, up) - BlackScholesPrice(type, dn)) /
             (2 * dx);
    };
    EXPECT_NEAR(g.delta, bump(&BlackScholesInputs::spot, h), 1e-7);
    EXPECT_NEAR(g.vega, bump(&BlackScholesInputs::volatility, h), 1e-5);
    EXPECT_NEAR(g.rho, bump(&BlackScholesInputs::rate, h), 1e-5);
    EXPECT_NEAR(g.theta, -bump(&BlackScholesInputs::time_to_expiry, h), 1e-5);
  }
}

TEST(CompoundGrowthTest, IsConstexpr) {
  static_assert(CompoundGrowth(0.0, 12, 5) == 1.0);
  constexpr double g = CompoundGrowth(0.10, 1, 2);
  EXPECT_NEAR(g, 1.21, 1e-15);
}

}  // namespace
}  // namespace cpp4q::hanson::ch01
