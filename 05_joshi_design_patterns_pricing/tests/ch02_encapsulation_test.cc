#include <initializer_list>
#include <gtest/gtest.h>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch02_encapsulation/payoff1.h"

namespace cpp4q::joshi::ch02 {
namespace {

TEST(PayOff1Test, Values) {
  const PayOff call(100, OptionType::kCall);
  const PayOff put(100, OptionType::kPut);
  EXPECT_EQ(call(120), 20);
  EXPECT_EQ(call(80), 0);
  EXPECT_EQ(put(80), 20);
  EXPECT_EQ(put(120), 0);
  EXPECT_EQ(call.strike(), 100);
  EXPECT_EQ(put.type(), OptionType::kPut);
}

TEST(PayOff1Test, McMatchesBlackScholes) {
  Rng rng(2);
  for (const auto type : {OptionType::kCall, OptionType::kPut}) {
    const McResult mc =
        SimpleMonteCarlo2(PayOff(95, type), 0.75, 100, 0.3, 0.03, 100'000, rng);
    const double exact = type == OptionType::kCall
                             ? BlackScholesCall(100, 95, 0.03, 0, 0.3, 0.75)
                             : BlackScholesPut(100, 95, 0.03, 0, 0.3, 0.75);
    EXPECT_NEAR(mc.price, exact, 4 * mc.std_error);
  }
}

}  // namespace
}  // namespace cpp4q::joshi::ch02
