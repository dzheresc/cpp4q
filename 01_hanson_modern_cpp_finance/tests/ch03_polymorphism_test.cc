#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "ch03_polymorphism/option.h"
#include "ch03_polymorphism/payoff.h"
#include "ch03_polymorphism/yield_curve.h"

namespace cpp4q::hanson::ch03 {
namespace {

TEST(PayoffTest, Values) {
  EXPECT_EQ(CallPayoff(100)(110), 10);
  EXPECT_EQ(CallPayoff(100)(90), 0);
  EXPECT_EQ(PutPayoff(100)(90), 10);
  EXPECT_EQ(DigitalCallPayoff(100, 5)(100.01), 5);
  EXPECT_EQ(DigitalCallPayoff(100, 5)(100), 0);
  EXPECT_EQ(DoubleDigitalPayoff(90, 110)(100), 1);
  EXPECT_EQ(DoubleDigitalPayoff(90, 110)(111), 0);
  EXPECT_THROW(DoubleDigitalPayoff(110, 90), std::invalid_argument);
}

TEST(PayoffTest, CloneIsPolymorphic) {
  const std::unique_ptr<Payoff> p = std::make_unique<PutPayoff>(50.0);
  const auto q = p->Clone();
  EXPECT_NE(p.get(), q.get());
  EXPECT_EQ((*q)(40.0), 10.0);
  EXPECT_NE(dynamic_cast<PutPayoff*>(q.get()), nullptr);
}

TEST(PayoffTest, SlicingIsPrevented) {
  static_assert(!std::is_copy_constructible_v<Payoff>);
  static_assert(std::has_virtual_destructor_v<Payoff>);
  static_assert(std::is_copy_constructible_v<CallPayoff>);
}

TEST(PayoffTest, VariantMatchesVirtual) {
  const std::vector<PayoffVariant> vs{Call{100}, Put{100}, DigitalCall{100, 2}};
  const CallPayoff c(100);
  const PutPayoff p(100);
  const DigitalCallPayoff d(100, 2);
  const std::vector<const Payoff*> ps{&c, &p, &d};
  for (double s : {80.0, 100.0, 120.0}) {
    for (std::size_t i = 0; i < vs.size(); ++i) {
      EXPECT_EQ(Evaluate(vs[i], s), (*ps[i])(s));
    }
  }
}

TEST(YieldCurveTest, FlatCurve) {
  const FlatYieldCurve c(0.03);
  EXPECT_NEAR(c.DiscountFactor(2.0), std::exp(-0.06), 1e-15);
  EXPECT_NEAR(c.ForwardRate(1.0, 3.0), 0.03, 1e-15);
}

TEST(YieldCurveTest, LinearInterpolationAndExtrapolation) {
  const LinearZeroCurve c({1.0, 2.0, 5.0}, {0.01, 0.02, 0.05});
  EXPECT_DOUBLE_EQ(c.ZeroRate(0.5), 0.01);
  EXPECT_DOUBLE_EQ(c.ZeroRate(1.5), 0.015);
  EXPECT_DOUBLE_EQ(c.ZeroRate(2.0), 0.02);
  EXPECT_NEAR(c.ZeroRate(3.5), 0.035, 1e-15);
  EXPECT_DOUBLE_EQ(c.ZeroRate(10.0), 0.05);
  EXPECT_THROW(LinearZeroCurve({1.0, 1.0}, {0.0, 0.0}), std::invalid_argument);
  EXPECT_THROW(LinearZeroCurve({1.0}, {}), std::invalid_argument);
}

TEST(OptionTest, OwnsPayoffSharesCurve) {
  auto curve = std::make_shared<const FlatYieldCurve>(0.05);
  Option a(std::make_unique<CallPayoff>(100.0), 1.0, curve);
  {
    const Option b = a;  // deep copies payoff, shares curve
    EXPECT_EQ(curve.use_count(), 3);
    EXPECT_NEAR(b.DiscountedPayoff(110.0), 10.0 * std::exp(-0.05), 1e-12);
  }
  EXPECT_EQ(curve.use_count(), 2);
  Option c(std::make_unique<PutPayoff>(1.0), 0.0, curve);
  c = a;
  EXPECT_EQ(c.PayoffAt(120.0), 20.0);
  EXPECT_THROW(Option(nullptr, 1.0, curve), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::hanson::ch03
