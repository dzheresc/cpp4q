#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>

#include "appendix_a_risk_neutral/binomial.h"
#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::appendix_a {
namespace {

TEST(OneStepTest, ReplicationIsExactAndEqualsRiskNeutralPrice) {
  const double s = 100, u = 1.2, d = 0.9, r = 0.05, dt = 1;
  const double up = 20, down = 0;  // call struck at 100
  const Replication rep = ReplicateOneStep(s, u, d, r, dt, up, down);
  const double growth = std::exp(r * dt);
  EXPECT_NEAR(rep.shares * s * u + rep.bond * growth, up, 1e-12);
  EXPECT_NEAR(rep.shares * s * d + rep.bond * growth, down, 1e-12);
  const double q = RiskNeutralProbability(u, d, r, dt);
  EXPECT_NEAR(rep.price, (q * up + (1 - q) * down) / growth, 1e-12);
  // The discounted stock is a q-martingale.
  EXPECT_NEAR((q * s * u + (1 - q) * s * d) / growth, s, 1e-12);
  EXPECT_THROW(RiskNeutralProbability(1.01, 0.9, 0.05, 1), std::domain_error);
}

TEST(CrrTest, MartingaleOnTheTree) {
  EXPECT_NEAR(CrrExpectation([](double s) { return s; }, 2, 100, 0.3, 0.04,
                             400),
              100, 1e-9);
  EXPECT_NEAR(CrrExpectation([](double) { return 1.0; }, 2, 100, 0.3, 0.04,
                             400),
              std::exp(-0.08), 1e-12);
}

TEST(CrrTest, ConvergesToBlackScholes) {
  const double bs = ch03::BlackScholesCallPrice(100, 1, 100, 0.2, 0.05);
  double previous = 1.0;
  for (int n : {25, 100, 400, 1600}) {
    const double err = std::abs(CrrPrice(PayoffType::kCall,
                                         ExerciseStyle::kEuropean, 100, 1, 100,
                                         0.2, 0.05, n) -
                                bs);
    EXPECT_LT(err, previous) << n;
    previous = err;
  }
  EXPECT_LT(previous, 2e-3);
  // European put-call parity holds exactly on the tree.
  const double c = CrrPrice(PayoffType::kCall, ExerciseStyle::kEuropean, 95, 1,
                            100, 0.2, 0.05, 101);
  const double p = CrrPrice(PayoffType::kPut, ExerciseStyle::kEuropean, 95, 1,
                            100, 0.2, 0.05, 101);
  EXPECT_NEAR(c - p, 100 - 95 * std::exp(-0.05), 1e-10);
}

TEST(CrrTest, AmericanExercise) {
  const auto price = [](PayoffType t, ExerciseStyle e) {
    return CrrPrice(t, e, 100, 1, 100, 0.2, 0.05, 500);
  };
  EXPECT_NEAR(price(PayoffType::kCall, ExerciseStyle::kAmerican),
              price(PayoffType::kCall, ExerciseStyle::kEuropean), 1e-12);
  EXPECT_GT(price(PayoffType::kPut, ExerciseStyle::kAmerican),
            price(PayoffType::kPut, ExerciseStyle::kEuropean) + 0.2);
  // Deep in the money, an American put is worth its intrinsic value.
  EXPECT_NEAR(CrrPrice(PayoffType::kPut, ExerciseStyle::kAmerican, 200, 1, 100,
                       0.2, 0.05, 200),
              100, 1e-12);
  EXPECT_THROW(CrrPrice(PayoffType::kPut, ExerciseStyle::kAmerican, 100, 1,
                        100, 0.2, 0.05, 0),
               std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::appendix_a
