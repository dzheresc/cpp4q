#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>

#include "ch01_getting_started/interest.h"

namespace cpp4q::armstrong::ch01 {
namespace {

TEST(InterestTest, SimpleAndCompound) {
  EXPECT_DOUBLE_EQ(SimpleInterest(1000, 0.05, 10), 1500);
  EXPECT_NEAR(CompoundInterest(1000, 0.05, 10, 1), 1000 * std::pow(1.05, 10),
              1e-9);
  EXPECT_NEAR(CompoundInterest(1000, 0.12, 1, 12),
              1000 * std::pow(1.01, 12), 1e-9);
}

TEST(InterestTest, CompoundingConvergesToContinuous) {
  const double cont = ContinuousCompounding(1000, 0.05, 10);
  double previous_gap = 1e300;
  for (int m : {1, 4, 12, 365, 100000}) {
    const double gap = cont - CompoundInterest(1000, 0.05, 10, m);
    EXPECT_GT(gap, 0);
    EXPECT_LT(gap, previous_gap);
    previous_gap = gap;
  }
  // The gap is P e^{rT} r^2 T / (2m) to leading order.
  EXPECT_NEAR(previous_gap / cont, 0.05 * 0.05 * 10 / (2 * 100000), 1e-10);
}

TEST(InterestTest, EquivalentRateAndDoubling) {
  const double rc = ContinuousEquivalentRate(0.05, 2);
  EXPECT_NEAR(ContinuousCompounding(1, rc, 7), CompoundInterest(1, 0.05, 7, 2),
              1e-14);
  EXPECT_NEAR(ContinuousCompounding(1, 0.07, DoublingTime(0.07)), 2.0, 1e-14);
  EXPECT_THROW(CompoundInterest(1, 0.05, 1, 0), std::invalid_argument);
  EXPECT_THROW(DoublingTime(0), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch01
