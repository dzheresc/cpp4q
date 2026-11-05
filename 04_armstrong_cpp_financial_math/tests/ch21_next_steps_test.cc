#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <vector>

#include "appendix_a_risk_neutral/binomial.h"
#include "ch03_functions/matlib.h"
#include "ch21_next_steps/pde.h"

namespace cpp4q::armstrong::ch21 {
namespace {

namespace bin = appendix_a;

TEST(TridiagonalTest, SolvesKnownSystem) {
  // [2 1 0; 1 3 1; 0 1 2] x = [3 5 3] -> x = [1 1 1].
  const std::vector<double> a{0, 1, 1}, b{2, 3, 2}, c{1, 1, 0};
  std::vector<double> d{3, 5, 3};
  SolveTridiagonal(a, b, c, d);
  for (double x : d) EXPECT_NEAR(x, 1.0, 1e-15);
}

TEST(PdeTest, EuropeanMatchesBlackScholes) {
  // Default grid: h = 1 in S.
  EXPECT_NEAR(PricePde(PayoffType::kCall, ExerciseStyle::kEuropean, 100, 1,
                       100, 0.2, 0.05)
                  .price,
              ch03::BlackScholesCallPrice(100, 1, 100, 0.2, 0.05), 3e-4);
  const PdeSettings fine{.n_space = 1600, .n_time = 800};
  for (double k : {80.0, 100.0, 120.0}) {
    const auto call = PricePde(PayoffType::kCall, ExerciseStyle::kEuropean, k,
                               1, 100, 0.2, 0.05, fine);
    const auto put = PricePde(PayoffType::kPut, ExerciseStyle::kEuropean, k, 1,
                              100, 0.2, 0.05, fine);
    EXPECT_NEAR(call.price, ch03::BlackScholesCallPrice(k, 1, 100, 0.2, 0.05),
                5e-5)
        << k;
    EXPECT_NEAR(put.price, ch03::BlackScholesPutPrice(k, 1, 100, 0.2, 0.05),
                5e-5)
        << k;
    EXPECT_NEAR(call.delta, ch03::BlackScholesCallDelta(k, 1, 100, 0.2, 0.05),
                1e-5);
    EXPECT_NEAR(call.gamma, ch03::BlackScholesGamma(k, 1, 100, 0.2, 0.05),
                2e-6);
  }
}

TEST(PdeTest, SecondOrderConvergence) {
  const double exact = ch03::BlackScholesCallPrice(100, 1, 100, 0.2, 0.05);
  const auto err = [&](int n) {
    return std::abs(PricePde(PayoffType::kCall, ExerciseStyle::kEuropean, 100,
                             1, 100, 0.2, 0.05, {.n_space = n, .n_time = n})
                        .price -
                    exact);
  };
  EXPECT_GT(err(100) / err(200), 3.0);  // ~4 for a second-order scheme
}

TEST(PdeTest, AmericanPutAgreesWithBinomialTree) {
  for (double k : {90.0, 100.0, 115.0}) {
    const double pde = PricePde(PayoffType::kPut, ExerciseStyle::kAmerican, k,
                                1, 100, 0.25, 0.06)
                           .price;
    const double tree = bin::CrrPrice(bin::PayoffType::kPut,
                                      bin::ExerciseStyle::kAmerican, k, 1, 100,
                                      0.25, 0.06, 2000);
    const double european = ch03::BlackScholesPutPrice(k, 1, 100, 0.25, 0.06);
    EXPECT_NEAR(pde, tree, 5e-3) << k;  // both have O(1e-3) error
    EXPECT_GT(pde, european + 0.05);  // a real early-exercise premium
    EXPECT_GE(pde, k - 100);
  }
}

TEST(PdeTest, AmericanCallWithoutDividendsIsEuropean) {
  const double am = PricePde(PayoffType::kCall, ExerciseStyle::kAmerican, 100,
                             1, 100, 0.2, 0.05)
                        .price;
  const double eu = PricePde(PayoffType::kCall, ExerciseStyle::kEuropean, 100,
                             1, 100, 0.2, 0.05)
                        .price;
  EXPECT_NEAR(am, eu, 1e-9);
}

TEST(PdeTest, RejectsBadInputs) {
  EXPECT_THROW(PricePde(PayoffType::kCall, ExerciseStyle::kEuropean, 100, 1,
                        100, 0, 0.05),
               std::invalid_argument);
  EXPECT_THROW(PricePde(PayoffType::kCall, ExerciseStyle::kEuropean, 100, 1,
                        100, 0.2, 0.05, {.n_space = 2}),
               std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch21
