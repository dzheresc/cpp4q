#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "ch03_functions/matlib.h"
#include "ch08_user_defined_types/options.h"

namespace cpp4q::armstrong::ch08 {
namespace {

TEST(OptionsTest, PayoffsAndAccessors) {
  const CallOption c(100, 1);
  const PutOption p(100, 1);
  EXPECT_EQ(c.strike(), 100);
  EXPECT_EQ(c.maturity(), 1);
  EXPECT_EQ(c.Payoff(110), 10);
  EXPECT_EQ(c.Payoff(90), 0);
  EXPECT_EQ(p.Payoff(90), 10);
  EXPECT_EQ(p.Payoff(110), 0);
  EXPECT_THROW(CallOption(0, 1), std::invalid_argument);
  EXPECT_THROW(PutOption(100, -1), std::invalid_argument);
}

TEST(OptionsTest, PriceUsesTimeToMaturity) {
  BlackScholesModel m;
  m.date = 0.5;
  const CallOption c(100, 1.5);
  EXPECT_DOUBLE_EQ(c.Price(m), ch03::BlackScholesCallPrice(100, 1, 100, 0.2,
                                                           0.05));
  const PutOption p(100, 1.5);
  EXPECT_NEAR(c.Price(m) - p.Price(m), 100 - 100 * std::exp(-0.05), 1e-12);
  m.date = 1.5;
  EXPECT_THROW(c.Price(m), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch08
