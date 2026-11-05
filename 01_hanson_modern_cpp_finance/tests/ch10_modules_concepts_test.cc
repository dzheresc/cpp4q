#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <list>
#include <stdexcept>
#include <vector>

#include "ch03_polymorphism/payoff.h"
#include "ch03_polymorphism/yield_curve.h"
#include "ch10_modules_concepts/concepts.h"
#include "ch10_modules_concepts/generic_pricing.h"

namespace cpp4q::hanson::ch10 {
namespace {

struct Flow {
  double time;
  double amount;
};
struct NotAFlow {
  double when;
};
struct UnitCurve {
  double DiscountFactor(double) const { return 1.0; }
};

// Compile-time checks: these are the real tests for a concepts chapter.
static_assert(Real<double> && Real<float> && !Real<int>);
static_assert(PayoffFunction<ch03::CallPayoff>);
static_assert(PayoffFunction<ch03::Call>);
static_assert(PayoffFunction<double (*)(double)>);
static_assert(!PayoffFunction<int>);
static_assert(DiscountCurve<ch03::FlatYieldCurve>);
static_assert(DiscountCurve<UnitCurve>);
static_assert(!DiscountCurve<Flow>);
static_assert(CashFlowRange<std::vector<Flow>>);
static_assert(CashFlowRange<std::list<Flow>>);
static_assert(!CashFlowRange<std::vector<NotAFlow>>);

constexpr TreeParams<double> kTree{.spot = 100.0,
                                   .rate = 0.05,
                                   .dividend_yield = 0.0,
                                   .volatility = 0.2,
                                   .expiry = 1.0,
                                   .steps = 2000};

TEST(BinomialTest, EuropeanConvergesToBlackScholes) {
  EXPECT_NEAR(
      PriceBinomial(kTree, ch03::CallPayoff(100.0), ExerciseStyle::kEuropean),
      10.450583572, 2e-3);
  EXPECT_NEAR(PriceBinomial(kTree, ch03::Put{100.0}, ExerciseStyle::kEuropean),
              5.573526022, 2e-3);
}

TEST(BinomialTest, AmericanPutPremium) {
  const auto put = [](double s) { return std::max(100.0 - s, 0.0); };
  const double am = PriceBinomial(kTree, put, ExerciseStyle::kAmerican);
  EXPECT_NEAR(am, 6.0904, 2e-3);  // reference ~6.0903
  // No dividends: early exercise of a call is never optimal.
  const ch03::CallPayoff call(100.0);
  EXPECT_NEAR(PriceBinomial(kTree, call, ExerciseStyle::kAmerican),
              PriceBinomial(kTree, call, ExerciseStyle::kEuropean), 1e-10);
}

TEST(BinomialTest, FloatInstantiationAndValidation) {
  const TreeParams<float> p{100.0f, 0.05f, 0.0f, 0.2f, 1.0f, 500};
  EXPECT_NEAR(PriceBinomial(p, ch03::Call{100.0}, ExerciseStyle::kEuropean),
              10.45f, 0.05f);
  TreeParams<double> bad = kTree;
  bad.steps = 0;
  EXPECT_THROW(
      (void)PriceBinomial(bad, ch03::Call{1.0}, ExerciseStyle::kEuropean),
      std::invalid_argument);
}

TEST(NpvTest, AnyRangeAnyCurve) {
  const std::list<Flow> flows{{1.0, 5.0}, {2.0, 105.0}};
  EXPECT_DOUBLE_EQ(Npv(flows, UnitCurve{}), 110.0);
  EXPECT_NEAR(Npv(flows, ch03::FlatYieldCurve(0.05)),
              5.0 * std::exp(-0.05) + 105.0 * std::exp(-0.1), 1e-12);
  EXPECT_EQ(PayoffAt(ch03::Put{10.0}, 4.0), 6.0);
}

}  // namespace
}  // namespace cpp4q::hanson::ch10
