#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/monte_carlo.h"
#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch01 {
namespace {

TEST(NormalTest, CdfKnownValues) {
  EXPECT_DOUBLE_EQ(NormalCdf(0.0), 0.5);
  EXPECT_NEAR(NormalCdf(1.959963984540054), 0.975, 1e-15);
  EXPECT_NEAR(NormalCdf(-8.0), 6.22096057427178e-16, 1e-28);
  EXPECT_NEAR(NormalPdf(0.0), 0.3989422804014327, 1e-16);
}

TEST(BlackScholesTest, TextbookValues) {
  const BlackScholesInputs in{};  // S=K=100, r=5%, vol=20%, T=1
  EXPECT_NEAR(BlackScholesPrice(OptionType::kCall, in), 10.450583572185565,
              1e-12);
  EXPECT_NEAR(BlackScholesPrice(OptionType::kPut, in), 5.573526022256971,
              1e-12);
  EXPECT_NEAR(BlackScholesDelta(OptionType::kCall, in), 0.6368306511756191,
              1e-12);
}

TEST(BlackScholesTest, PutCallParity) {
  const BlackScholesInputs in{.spot = 95,
                              .strike = 105,
                              .rate = 0.03,
                              .vol = 0.3,
                              .maturity = 2,
                              .dividend = 0.01};
  const double c = BlackScholesPrice(OptionType::kCall, in);
  const double p = BlackScholesPrice(OptionType::kPut, in);
  EXPECT_NEAR(c - p, 95 * std::exp(-0.02) - 105 * std::exp(-0.06), 1e-12);
}

TEST(BlackScholesTest, GreeksMatchFiniteDifferences) {
  BlackScholesInputs in{.spot = 90, .strike = 100, .vol = 0.25};
  const double h = 1e-3;
  auto price_at = [&](double s, double v) {
    BlackScholesInputs x = in;
    x.spot = s;
    x.vol = v;
    return BlackScholesPrice(OptionType::kCall, x);
  };
  EXPECT_NEAR(BlackScholesDelta(OptionType::kCall, in),
              (price_at(90 + h, 0.25) - price_at(90 - h, 0.25)) / (2 * h),
              1e-7);
  EXPECT_NEAR(BlackScholesGamma(in),
              (price_at(90 + h, 0.25) - 2 * price_at(90, 0.25) +
               price_at(90 - h, 0.25)) /
                  (h * h),
              1e-5);
  EXPECT_NEAR(BlackScholesVega(in),
              (price_at(90, 0.25 + h) - price_at(90, 0.25 - h)) / (2 * h),
              1e-4);
  in.spot = 90 + h;
  const double up = DigitalCallPrice(in);
  in.spot = 90 - h;
  const double down = DigitalCallPrice(in);
  in.spot = 90;
  EXPECT_NEAR(DigitalCallDelta(in), (up - down) / (2 * h), 1e-8);
}

TEST(BlackScholesTest, RejectsBadInputs) {
  EXPECT_THROW(BlackScholesPrice(OptionType::kCall, {.vol = 0.0}),
               std::invalid_argument);
  EXPECT_THROW(GeometricAsianCallPrice({}, 0), std::invalid_argument);
}

TEST(GeometricAsianTest, OneFixingIsEuropean) {
  const BlackScholesInputs in{};
  EXPECT_NEAR(GeometricAsianCallPrice(in, 1),
              BlackScholesPrice(OptionType::kCall, in), 1e-12);
}

TEST(GeometricAsianTest, BelowArithmeticAsian) {
  const BlackScholesInputs in{};
  const Estimate arith = PriceArithmeticAsianCall(in, 12, 200'000, 5);
  EXPECT_LT(GeometricAsianCallPrice(in, 12), arith.Lower(4.0));
}

TEST(RunningStatsTest, MatchesTwoPassAndMerges) {
  const double xs[] = {1.0, 4.0, 2.5, -3.0, 10.0, 0.5, 7.25};
  RunningStats all, left, right;
  double sum = 0.0;
  for (int i = 0; i < 7; ++i) {
    all.Add(xs[i]);
    (i < 3 ? left : right).Add(xs[i]);
    sum += xs[i];
  }
  const double mean = sum / 7;
  double ss = 0.0;
  for (double x : xs) ss += (x - mean) * (x - mean);
  EXPECT_NEAR(all.Mean(), mean, 1e-14);
  EXPECT_NEAR(all.Variance(), ss / 6, 1e-12);
  left.Merge(right);
  EXPECT_EQ(left.count(), 7u);
  EXPECT_NEAR(left.Mean(), mean, 1e-14);
  EXPECT_NEAR(left.Variance(), ss / 6, 1e-12);
  RunningStats empty;
  empty.Merge(all);
  EXPECT_DOUBLE_EQ(empty.Mean(), all.Mean());
}

TEST(EstimateTest, Interval) {
  const Estimate e{10.0, 0.5, 100};
  EXPECT_DOUBLE_EQ(e.HalfWidth(), 0.98);
  EXPECT_TRUE(e.Contains(10.9));
  EXPECT_FALSE(e.Contains(11.0));
}

TEST(MonteCarloTest, CallWithinConfidenceInterval) {
  const BlackScholesInputs in{};
  const Estimate e = PriceEuropeanCall(in, 400'000, 99);
  EXPECT_TRUE(e.Contains(BlackScholesPrice(OptionType::kCall, in), 4.0));
  EXPECT_NEAR(e.std_error, 14.7 / std::sqrt(400'000.0), 1e-3);
}

TEST(MonteCarloTest, GenericPayoffPut) {
  const BlackScholesInputs in{.strike = 110};
  const Estimate e = PriceEuropean(in, PutPayoff{110}, 200'000, 3);
  EXPECT_TRUE(e.Contains(BlackScholesPrice(OptionType::kPut, in), 4.0));
}

TEST(MonteCarloTest, ErrorShrinksLikeInverseSqrtN) {
  const auto study = ConvergenceStudy({}, 8, 16, 1);
  EXPECT_NEAR(StdErrorSlope(study), -0.5, 0.03);
}

TEST(MonteCarloTest, WorkNormalizedVariance) {
  // Half the variance at triple the cost is worse.
  EXPECT_LT(WorkNormalizedVariance(1.0, 1.0), WorkNormalizedVariance(0.5, 3.0));
}

}  // namespace
}  // namespace cpp4q::glasserman::ch01
