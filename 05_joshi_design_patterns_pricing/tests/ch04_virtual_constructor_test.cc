#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch04_virtual_constructor/vanilla.h"

namespace cpp4q::joshi::ch04 {
namespace {

static_assert(std::is_nothrow_move_constructible_v<PayOffBridge>);
static_assert(std::is_nothrow_move_constructible_v<VanillaOption>);

TEST(VirtualConstructorTest, CloneKeepsDynamicType) {
  const PayOffDoubleDigital dd(90, 110);
  const PayOff& base = dd;
  const std::unique_ptr<PayOff> copy = base.Clone();
  EXPECT_NE(copy.get(), &base);
  EXPECT_NE(dynamic_cast<PayOffDoubleDigital*>(copy.get()), nullptr);
  EXPECT_EQ((*copy)(100), 1);
}

TEST(VirtualConstructorTest, BatchEvaluateMatchesPointwise) {
  const std::vector<double> spots{50, 89.9, 90, 100, 110, 110.1, 200};
  std::vector<double> values(spots.size());
  const PayOffDoubleDigital dd(90, 110);
  const PayOffPut put(100);
  for (const PayOff* p : {static_cast<const PayOff*>(&dd),
                          static_cast<const PayOff*>(&put)}) {
    p->Evaluate(spots, values);
    for (std::size_t i = 0; i < spots.size(); ++i) {
      EXPECT_EQ(values[i], (*p)(spots[i]));
    }
  }
}

TEST(VirtualConstructorTest, BridgeDeepCopies) {
  PayOffBridge a = PayOffCall(100);
  PayOffBridge b = a;
  EXPECT_NE(&a.get(), &b.get());
  EXPECT_EQ(b(150), 50);
  b = PayOffPut(100);
  EXPECT_EQ(a(150), 50);  // a unaffected
  EXPECT_EQ(b(50), 50);
  const PayOffBridge& self = b;
  b = self;  // self-assignment is safe
  EXPECT_EQ(b(50), 50);
  EXPECT_THROW(PayOffBridge(std::unique_ptr<PayOff>()), std::invalid_argument);
}

TEST(VirtualConstructorTest, OptionsCopyThroughTheBridge) {
  VanillaOption original(PayOffDigitalCall(100), 2.0);
  const VanillaOption copy = original;
  original = VanillaOption(PayOffPut(100), 1.0);
  EXPECT_EQ(copy.expiry(), 2.0);
  EXPECT_EQ(copy.OptionPayOff(101), 1);
  EXPECT_EQ(original.OptionPayOff(101), 0);
}

TEST(ParametersTest, ConstantAndPiecewise) {
  const Parameters c = 0.3;
  EXPECT_DOUBLE_EQ(c.Integral(0.5, 2.0), 0.45);
  EXPECT_DOUBLE_EQ(c.RootMeanSquare(1, 3), 0.3);
  const Parameters p = ParametersPiecewiseConstant({1, 2}, {0.1, 0.2, 0.4});
  EXPECT_DOUBLE_EQ(p.Integral(0, 1), 0.1);
  EXPECT_DOUBLE_EQ(p.Integral(0.5, 2.5), 0.05 + 0.2 + 0.2);
  EXPECT_DOUBLE_EQ(p.IntegralSquare(0, 3), 0.01 + 0.04 + 0.16);
  EXPECT_DOUBLE_EQ(p.Mean(1, 2), 0.2);
  EXPECT_NEAR(p.RootMeanSquare(0, 3), std::sqrt(0.21 / 3), 1e-15);
  EXPECT_THROW(ParametersPiecewiseConstant({1, 1}, {1, 2, 3}),
               std::invalid_argument);
  EXPECT_THROW(ParametersPiecewiseConstant({1}, {1}), std::invalid_argument);
}

TEST(VanillaMcTest, TermStructurePricesLikeRmsVol) {
  const Parameters vol = ParametersPiecewiseConstant({0.5}, {0.35, 0.15});
  const Parameters r = ParametersPiecewiseConstant({0.25}, {0.02, 0.06});
  const VanillaOption call(PayOffCall(100), 1.0);
  Rng rng(4);
  const McResult mc = SimpleMonteCarlo4(call, 100, vol, r, 200'000, rng);
  const double exact = BlackScholesCall(100, 100, r.Mean(0, 1), 0,
                                        vol.RootMeanSquare(0, 1), 1);
  EXPECT_NEAR(mc.price, exact, 4 * mc.std_error);
}

}  // namespace
}  // namespace cpp4q::joshi::ch04
