#include <gtest/gtest.h>

#include <stdexcept>

#include "ch01_foundations/black_scholes.h"
#include "ch04_variance_reduction/variance_reduction.h"

namespace cpp4q::glasserman::ch04 {
namespace {

using ch02::Mrg32k3a;
using ch02::Sampler;

double CallPrice(const BlackScholesInputs& in) {
  return ch01::BlackScholesPrice(ch01::OptionType::kCall, in);
}

// Variance per payoff evaluation.
double CostVariance(const Estimate& e, double evals_per_sample = 1.0) {
  return e.std_error * e.std_error * static_cast<double>(e.samples) *
         evals_per_sample;
}

TEST(ControlVariateAccumulatorTest, PerfectControlRemovesVariance) {
  ControlVariateAccumulator acc;
  for (int i = 0; i < 100; ++i) {
    const double x = i * 0.37 - 5.0;
    acc.Add(3.0 * x + 1.0, x);
  }
  EXPECT_NEAR(acc.Beta(), 3.0, 1e-12);
  EXPECT_NEAR(acc.Correlation(), 1.0, 1e-12);
  const Estimate e = acc.Result(0.0);  // E[X] = 0 -> E[Y] = 1
  EXPECT_NEAR(e.mean, 1.0, 1e-10);
  EXPECT_NEAR(e.std_error, 0.0, 1e-7);
  ControlVariateAccumulator small;
  small.Add(1, 1);
  EXPECT_THROW(small.Result(0.0), std::logic_error);
}

TEST(VarianceReductionTest, AllEuropeanEstimatorsUnbiased) {
  const BlackScholesInputs in{};
  const double exact = CallPrice(in);
  Sampler s(Mrg32k3a(1));
  EXPECT_TRUE(PlainCall(in, 200'000, s).Contains(exact, 4.0));
  EXPECT_TRUE(AntitheticCall(in, 100'000, s).Contains(exact, 4.0));
  EXPECT_TRUE(ControlVariateCall(in, 200'000, s).estimate.Contains(exact, 4.0));
  EXPECT_TRUE(StratifiedCall(in, 50, 400, s).Contains(exact, 4.0));
  EXPECT_TRUE(ImportanceSamplingCall(in, 0.3, 200'000, s).Contains(exact, 4.0));
}

TEST(VarianceReductionTest, EachTechniqueReducesVariance) {
  const BlackScholesInputs in{};
  Sampler s(Mrg32k3a(2));
  const double plain = CostVariance(PlainCall(in, 100'000, s));
  EXPECT_LT(CostVariance(AntitheticCall(in, 50'000, s), 2.0), 0.7 * plain);
  const auto cv = ControlVariateCall(in, 100'000, s);
  EXPECT_LT(CostVariance(cv.estimate), 0.3 * plain);
  EXPECT_GT(cv.correlation, 0.85);
  EXPECT_LT(CostVariance(StratifiedCall(in, 100, 1000, s)), 0.02 * plain);
}

TEST(VarianceReductionTest, ImportanceSamplingDeepOutOfTheMoney) {
  BlackScholesInputs in{};
  in.strike = 170.0;
  const double exact = CallPrice(in);
  Sampler s(Mrg32k3a(3));
  const double shift = StrikeCenteringShift(in);
  EXPECT_GT(shift, 2.0);
  const Estimate plain = PlainCall(in, 100'000, s);
  const Estimate is = ImportanceSamplingCall(in, shift, 100'000, s);
  EXPECT_TRUE(is.Contains(exact, 4.0));
  EXPECT_LT(is.std_error, plain.std_error / 10.0);
}

TEST(VarianceReductionTest, StratificationNeedsTwoPerStratum) {
  Sampler s;
  EXPECT_THROW(StratifiedCall({}, 10, 1, s), std::invalid_argument);
  EXPECT_THROW(TerminalStratifiedAsian({}, 0, 5, s), std::invalid_argument);
}

TEST(AsianTest, EstimatorsAgreeAndGeometricControlIsStrong) {
  const AsianSpec spec{{}, 12};
  Sampler s(Mrg32k3a(4));
  const Estimate plain = PlainAsian(spec, 100'000, s);
  const Estimate anti = AntitheticAsian(spec, 50'000, s);
  const auto cv = GeometricControlAsian(spec, 100'000, s);
  const Estimate strat = TerminalStratifiedAsian(spec, 50, 200, s);
  // The control-variate estimate is accurate to ~1e-3: use it as truth.
  const double truth = cv.estimate.mean;
  EXPECT_NEAR(plain.mean, truth, 4 * plain.std_error);
  EXPECT_NEAR(anti.mean, truth, 4 * anti.std_error);
  EXPECT_NEAR(strat.mean, truth, 4 * strat.std_error);
  EXPECT_GT(cv.correlation, 0.99);
  EXPECT_NEAR(cv.beta, 1.0, 0.1);
  EXPECT_LT(CostVariance(cv.estimate), CostVariance(plain) / 100.0);
  EXPECT_LT(CostVariance(strat), CostVariance(plain));
}

TEST(AsianTest, OneFixingGeometricControlIsExact) {
  // With one fixing the arithmetic and geometric payoffs coincide.
  const AsianSpec spec{{}, 1};
  Sampler s(Mrg32k3a(5));
  const auto cv = GeometricControlAsian(spec, 1000, s);
  EXPECT_NEAR(cv.estimate.mean, CallPrice({}), 1e-9);
}

}  // namespace
}  // namespace cpp4q::glasserman::ch04
