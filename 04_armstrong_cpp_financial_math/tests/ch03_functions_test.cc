#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch03 {
namespace {

TEST(NormalTest, KnownValues) {
  EXPECT_DOUBLE_EQ(NormCdf(0), 0.5);
  EXPECT_NEAR(NormCdf(1.959963984540054), 0.975, 1e-15);
  EXPECT_NEAR(NormCdf(-8), 6.22096057427178e-16, 1e-28);
  EXPECT_NEAR(NormPdf(0), 0.3989422804014327, 1e-16);
}

TEST(NormalTest, PolynomialWithinPublishedBound) {
  double worst = 0;
  for (double x = -8; x <= 8; x += 0.001) {
    worst = std::max(worst, std::abs(NormCdfPolynomial(x) - NormCdf(x)));
  }
  EXPECT_LT(worst, 7.5e-8);
  EXPECT_GT(worst, 1e-9);  // it really is an approximation
}

TEST(NormalTest, InverseRoundTrips) {
  for (double p : {1e-300, 1e-12, 1e-4, 0.02, 0.02425, 0.1, 0.5, 0.7, 0.97575,
                   0.999, 1 - 1e-12}) {
    const double x = NormInv(p);
    const double back = p <= 0.5 ? NormCdf(x) : 1 - NormCdf(-x);
    EXPECT_NEAR(back / p, 1.0, 1e-13) << p;
  }
  EXPECT_DOUBLE_EQ(NormInv(0.5), 0.0);
  EXPECT_NEAR(NormInv(0.975), 1.959963984540054, 1e-14);
  EXPECT_THROW(NormInv(0), std::domain_error);
  EXPECT_THROW(NormInv(1), std::domain_error);
  EXPECT_THROW(NormInv(std::nan("")), std::domain_error);
}

TEST(BlackScholesTest, TextbookValues) {
  EXPECT_NEAR(BlackScholesCallPrice(100, 1, 100, 0.2, 0.05), 10.450583572185565,
              1e-12);
  EXPECT_NEAR(BlackScholesPutPrice(100, 1, 100, 0.2, 0.05), 5.573526022256971,
              1e-12);
  EXPECT_NEAR(BlackScholesCallDelta(100, 1, 100, 0.2, 0.05),
              0.6368306511756191, 1e-12);
}

TEST(BlackScholesTest, ParityAndGreeksByDifferences) {
  const double k = 110, t = 0.75, s = 95, v = 0.3, r = 0.02;
  EXPECT_NEAR(BlackScholesCallPrice(k, t, s, v, r) -
                  BlackScholesPutPrice(k, t, s, v, r),
              s - k * std::exp(-r * t), 1e-12);
  const double h = 1e-3;
  const double c_up = BlackScholesCallPrice(k, t, s + h, v, r);
  const double c_dn = BlackScholesCallPrice(k, t, s - h, v, r);
  const double c = BlackScholesCallPrice(k, t, s, v, r);
  EXPECT_NEAR(BlackScholesCallDelta(k, t, s, v, r), (c_up - c_dn) / (2 * h),
              1e-8);
  EXPECT_NEAR(BlackScholesGamma(k, t, s, v, r),
              (c_up - 2 * c + c_dn) / (h * h), 1e-5);
  EXPECT_NEAR(BlackScholesVega(k, t, s, v, r),
              (BlackScholesCallPrice(k, t, s, v + h, r) -
               BlackScholesCallPrice(k, t, s, v - h, r)) /
                  (2 * h),
              1e-4);  // O(h^2) truncation error, h = 1e-3
}

TEST(BlackScholesTest, RejectsBadInputs) {
  EXPECT_THROW(BlackScholesCallPrice(100, 1, 100, 0, 0.05),
               std::invalid_argument);
  EXPECT_THROW(BlackScholesPutPrice(100, 0, 100, 0.2, 0.05),
               std::invalid_argument);
  EXPECT_THROW(BlackScholesCallPrice(-1, 1, 100, 0.2, 0.05),
               std::invalid_argument);
}

TEST(QuadraticTest, StableRoots) {
  const auto r = SolveQuadratic(1, -1e8, 1);
  ASSERT_TRUE(r);
  EXPECT_NEAR(r->first, 1e-8, 1e-22);  // the naive formula gives ~7.45e-9
  EXPECT_NEAR(r->second, 1e8, 1e-6);
  const auto two = SolveQuadratic(2, 0, -8);
  ASSERT_TRUE(two);
  EXPECT_DOUBLE_EQ(two->first, -2);
  EXPECT_DOUBLE_EQ(two->second, 2);
  EXPECT_FALSE(SolveQuadratic(1, 0, 1));
  EXPECT_FALSE(SolveQuadratic(0, 1, 1));
  const auto zero = SolveQuadratic(1, 0, 0);
  ASSERT_TRUE(zero);
  EXPECT_EQ(zero->first, 0);
  EXPECT_EQ(Square(3), 9);
  EXPECT_DOUBLE_EQ(Square(1.5), 2.25);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch03
