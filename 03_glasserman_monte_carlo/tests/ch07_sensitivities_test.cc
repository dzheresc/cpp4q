#include <gtest/gtest.h>

#include <cmath>

#include "ch01_foundations/black_scholes.h"
#include "ch07_sensitivities/greeks.h"

namespace cpp4q::glasserman::ch07 {
namespace {

using ch01::OptionType;

TEST(FiniteDifferenceTest, CommonRandomNumbersCutVariance) {
  const BlackScholesInputs in{};
  const double delta = ch01::BlackScholesDelta(OptionType::kCall, in);
  const auto indep = FiniteDifferenceDelta(in, 0.01, FiniteDifference::kCentral,
                                           false, 50'000, 1);
  const auto crn = FiniteDifferenceDelta(in, 0.01, FiniteDifference::kCentral,
                                         true, 50'000, 1);
  EXPECT_LT(crn.std_error, indep.std_error / 10.0);
  EXPECT_TRUE(crn.Contains(delta, 4.0));
  EXPECT_TRUE(indep.Contains(delta, 4.0));
}

TEST(FiniteDifferenceTest, ForwardDifferenceIsBiased) {
  const BlackScholesInputs in{};
  const double delta = ch01::BlackScholesDelta(OptionType::kCall, in);
  // Bias ~ h/2 * gamma * S = 0.5 * 0.0188 * 10 ~ 0.09 at a 10% bump.
  const auto fwd = FiniteDifferenceDelta(in, 0.1, FiniteDifference::kForward,
                                         true, 200'000, 2);
  EXPECT_GT(fwd.mean - delta, 8 * fwd.std_error);
  const auto ctr = FiniteDifferenceDelta(in, 0.1, FiniteDifference::kCentral,
                                         true, 200'000, 2);
  EXPECT_LT(std::abs(ctr.mean - delta), std::abs(fwd.mean - delta) / 5.0);
}

TEST(CallGreeksTest, AllEstimatorsUnbiased) {
  const BlackScholesInputs in{.spot = 95, .strike = 100, .vol = 0.3};
  const auto g = EuropeanCallGreeks(in, 400'000, 3);
  EXPECT_TRUE(
      g.price.Contains(ch01::BlackScholesPrice(OptionType::kCall, in), 4));
  const double delta = ch01::BlackScholesDelta(OptionType::kCall, in);
  const double vega = ch01::BlackScholesVega(in);
  EXPECT_TRUE(g.pathwise_delta.Contains(delta, 4));
  EXPECT_TRUE(g.lr_delta.Contains(delta, 4));
  EXPECT_TRUE(g.pathwise_vega.Contains(vega, 4));
  EXPECT_TRUE(g.lr_vega.Contains(vega, 4));
  EXPECT_TRUE(g.lr_gamma.Contains(ch01::BlackScholesGamma(in), 4));
}

TEST(CallGreeksTest, PathwiseHasLowerVarianceThanLikelihoodRatio) {
  const auto g = EuropeanCallGreeks({}, 100'000, 4);
  EXPECT_LT(g.pathwise_delta.std_error, g.lr_delta.std_error);
  EXPECT_LT(g.pathwise_vega.std_error, g.lr_vega.std_error);
}

TEST(DigitalGreeksTest, OnlyLikelihoodRatioWorks) {
  const BlackScholesInputs in{.strike = 105};
  const auto g = DigitalCallGreeks(in, 400'000, 5);
  const double exact = ch01::DigitalCallDelta(in);
  EXPECT_TRUE(g.price.Contains(ch01::DigitalCallPrice(in), 4));
  EXPECT_TRUE(g.lr_delta.Contains(exact, 4));
  EXPECT_EQ(g.pathwise_delta.mean, 0.0);
  EXPECT_GT(exact, 0.01);
}

TEST(AsianDeltaTest, PathwiseAndLikelihoodRatioAgree) {
  const BlackScholesInputs in{};
  const auto d = AsianCallDelta(in, 12, 200'000, 6);
  EXPECT_NEAR(d.pathwise_delta.mean, d.lr_delta.mean,
              4 * std::hypot(d.pathwise_delta.std_error, d.lr_delta.std_error));
  EXPECT_LT(d.pathwise_delta.std_error, d.lr_delta.std_error);
  // One fixing at T: the European delta.
  const auto one = AsianCallDelta(in, 1, 200'000, 7);
  EXPECT_TRUE(one.pathwise_delta.Contains(
      ch01::BlackScholesDelta(OptionType::kCall, in), 4));
}

}  // namespace
}  // namespace cpp4q::glasserman::ch07
