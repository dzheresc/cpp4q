#include <initializer_list>
#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "appendix_b_distributions/normals.h"

namespace cpp4q::joshi {
namespace {

TEST(NormalsTest, CdfKnownValues) {
  EXPECT_DOUBLE_EQ(CumulativeNormal(0.0), 0.5);
  EXPECT_NEAR(CumulativeNormal(1.96), 0.9750021048517795, 1e-15);
  EXPECT_NEAR(CumulativeNormal(-1.0), 0.15865525393145707, 1e-15);
  // erfc keeps relative precision far in the lower tail.
  EXPECT_NEAR(CumulativeNormal(-10.0) / 7.619853024160527e-24, 1.0, 1e-13);
}

TEST(NormalsTest, DensityIntegratesToCdfDerivative) {
  for (const double x : {-3.0, -0.5, 0.0, 1.2, 4.0}) {
    const double h = 1e-5;
    EXPECT_NEAR((CumulativeNormal(x + h) - CumulativeNormal(x - h)) / (2 * h),
                NormalDensity(x), 1e-10);
  }
}

TEST(NormalsTest, MoroInverseAccuracy) {
  // Beasley-Springer-Moro: |x - N^{-1}(u)| < 3e-9 on [1e-10, 1 - 1e-10]; the
  // refined version is accurate to rounding (checked through the cdf).
  double worst = 0.0;
  for (int i = 1; i < 20000; ++i) {
    const double u = i / 20000.0;
    const double x = InverseCumulativeNormal(u);
    const double exact = InverseCumulativeNormalRefined(u);
    worst = std::max(worst, std::fabs(x - exact));
    EXPECT_NEAR(CumulativeNormal(exact), u, 4e-16) << u;
  }
  EXPECT_LT(worst, 3e-9);
  for (const double u : {1e-10, 1e-7, 1 - 1e-7, 1 - 1e-10}) {
    EXPECT_NEAR(InverseCumulativeNormal(u), InverseCumulativeNormalRefined(u),
                3e-9 * std::fabs(InverseCumulativeNormalRefined(u)));
  }
}

TEST(NormalsTest, InverseEdgeCases) {
  EXPECT_EQ(InverseCumulativeNormal(0.0),
            -std::numeric_limits<double>::infinity());
  EXPECT_EQ(InverseCumulativeNormal(1.0),
            std::numeric_limits<double>::infinity());
  EXPECT_TRUE(std::isnan(InverseCumulativeNormal(-0.1)));
  EXPECT_TRUE(std::isnan(InverseCumulativeNormal(std::nan(""))));
  EXPECT_DOUBLE_EQ(InverseCumulativeNormal(0.5), 0.0);
  // Odd symmetry.
  EXPECT_NEAR(InverseCumulativeNormal(0.025), -InverseCumulativeNormal(0.975),
              1e-12);
}

}  // namespace
}  // namespace cpp4q::joshi
