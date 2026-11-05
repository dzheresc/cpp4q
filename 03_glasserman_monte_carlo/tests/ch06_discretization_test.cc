#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch03_sample_paths/short_rate.h"
#include "ch06_discretization/schemes.h"

namespace cpp4q::glasserman::ch06 {
namespace {

TEST(StepTest, EulerAndMilsteinFormulas) {
  auto a = [](double, double x) { return 0.1 * x; };
  auto b = [](double, double x) { return 0.2 * x; };
  auto db = [](double, double) { return 0.2; };
  EXPECT_DOUBLE_EQ(EulerStep(a, b, 0.0, 100.0, 0.01, 0.05), 100.0 + 0.1 + 1.0);
  EXPECT_DOUBLE_EQ(MilsteinStep(a, b, db, 0.0, 100.0, 0.01, 0.05),
                   101.1 + 0.5 * 20.0 * 0.2 * (0.0025 - 0.01));
}

TEST(StrongOrderTest, EulerHalfMilsteinOne) {
  const ch03::GbmParams p{100.0, 0.05, 0.4};
  const std::vector<std::size_t> steps = {8, 16, 32, 64};
  std::vector<double> euler, milstein;
  for (std::size_t n : steps) {
    euler.push_back(GbmStrongError(p, 1.0, n, Scheme::kEuler, 10'000, 1).mean);
    milstein.push_back(
        GbmStrongError(p, 1.0, n, Scheme::kMilstein, 10'000, 1).mean);
  }
  EXPECT_NEAR(FitOrder(steps, euler), 0.5, 0.1);
  EXPECT_NEAR(FitOrder(steps, milstein), 1.0, 0.15);
  for (std::size_t i = 0; i < steps.size(); ++i) {
    EXPECT_LT(milstein[i], euler[i]);
  }
}

TEST(WeakOrderTest, EulerMomentsConvergeLinearly) {
  const ch03::GbmParams p{100.0, 0.05, 0.4};
  const double exact1 = 100.0 * std::exp(0.05);
  const double exact2 = 1e4 * std::exp(0.1 + 0.16);
  for (std::size_t n : {4u, 16u, 64u}) {
    const double e1 = std::abs(EulerGbmMean(p, 1.0, n) - exact1);
    const double e1_half = std::abs(EulerGbmMean(p, 1.0, 2 * n) - exact1);
    EXPECT_NEAR(e1 / e1_half, 2.0, 0.1);  // halving h halves the error
    const double e2 = EulerGbmSecondMoment(p, 1.0, n) - exact2;
    const double e2_half = EulerGbmSecondMoment(p, 1.0, 2 * n) - exact2;
    EXPECT_NEAR(e2 / e2_half, 2.0, 0.15);
    // Richardson removes the O(h) term: much smaller error.
    const double rich = 2 * EulerGbmSecondMoment(p, 1.0, 2 * n) -
                        EulerGbmSecondMoment(p, 1.0, n) - exact2;
    EXPECT_LT(std::abs(rich), 0.2 * std::abs(e2_half));
  }
}

TEST(WeakOrderTest, EulerCallPricesBothIncrementTypes) {
  const ch01::BlackScholesInputs in{};
  const double exact = ch01::BlackScholesPrice(ch01::OptionType::kCall, in);
  const auto normal = EulerCallPrice(in, 32, 100'000, false, 2);
  const auto two_point = EulerCallPrice(in, 32, 100'000, true, 3);
  // At 32 steps the weak bias is well inside the noise.
  EXPECT_TRUE(normal.Contains(exact, 4.0));
  EXPECT_TRUE(two_point.Contains(exact, 4.0));
  const auto rich = RichardsonCallPrice(in, 16, 100'000, 4);
  EXPECT_TRUE(rich.Contains(exact, 4.0));
}

TEST(CirEulerTest, FullTruncationLeastBiased) {
  const ch03::ShortRateParams p{.kappa = 0.2, .theta = 0.04, .sigma = 0.3};
  const double exact = ch03::CirBondPrice(p, 0.02, 5.0);
  const auto absorb =
      CirEulerBond(p, 0.02, 5.0, 20, CirFix::kAbsorb, 50'000, 5);
  const auto reflect =
      CirEulerBond(p, 0.02, 5.0, 20, CirFix::kReflect, 50'000, 5);
  const auto full =
      CirEulerBond(p, 0.02, 5.0, 20, CirFix::kFullTruncation, 50'000, 5);
  EXPECT_LT(std::abs(full.mean - exact), std::abs(absorb.mean - exact));
  EXPECT_LT(std::abs(absorb.mean - exact), std::abs(reflect.mean - exact));
  // More steps shrink the bias.
  const auto fine =
      CirEulerBond(p, 0.02, 5.0, 200, CirFix::kFullTruncation, 50'000, 6);
  EXPECT_LT(std::abs(fine.mean - exact), std::abs(full.mean - exact));
}

}  // namespace
}  // namespace cpp4q::glasserman::ch06
