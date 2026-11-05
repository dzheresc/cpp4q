#include <initializer_list>
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch01_simple_monte_carlo/simple_mc.h"

namespace cpp4q::joshi::ch01 {
namespace {

TEST(SimpleMcTest, CallWithinFourStandardErrors) {
  Rng rng(1);
  const McResult mc = SimpleMonteCarlo1(1.0, 105, 100, 0.2, 0.05, 200'000, rng);
  const double exact = BlackScholesCall(100, 105, 0.05, 0, 0.2, 1.0);
  EXPECT_NEAR(mc.price, exact, 4 * mc.std_error);
  EXPECT_LT(mc.std_error, 0.04);
}

TEST(SimpleMcTest, Reproducible) {
  Rng a(9), b(9);
  EXPECT_EQ(SimpleMonteCarlo1(1, 100, 100, 0.2, 0.05, 1000, a).price,
            SimpleMonteCarlo1(1, 100, 100, 0.2, 0.05, 1000, b).price);
}

TEST(SimpleMcTest, UniformIsOpen) {
  Rng rng(3);
  for (int i = 0; i < 100'000; ++i) {
    const double u = UniformOpen(rng);
    ASSERT_GT(u, 0.0);
    ASSERT_LT(u, 1.0);
  }
}

// First four moments of each Gaussian generator. Summation of 12 uniforms
// has the right variance but kurtosis -0.1 instead of 0.
TEST(SimpleMcTest, GaussianMoments) {
  using Gen = double (*)(Rng&);
  const int n = 400'000;
  for (const Gen gen : {&GetOneGaussianByBoxMuller, &GetOneGaussianByPolar,
                        &GetOneGaussianBySummation}) {
    Rng rng(7);
    double m1 = 0, m2 = 0, m4 = 0;
    for (int i = 0; i < n; ++i) {
      const double z = gen(rng);
      m1 += z;
      m2 += z * z;
      m4 += z * z * z * z;
    }
    m1 /= n;
    m2 /= n;
    m4 /= n;
    EXPECT_NEAR(m1, 0.0, 4 / std::sqrt(n));
    EXPECT_NEAR(m2, 1.0, 4 * std::sqrt(2.0 / n));
    const double expected_m4 = gen == &GetOneGaussianBySummation ? 2.9 : 3.0;
    EXPECT_NEAR(m4, expected_m4, 4 * std::sqrt(96.0 / n));
  }
}

}  // namespace
}  // namespace cpp4q::joshi::ch01
