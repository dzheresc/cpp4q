#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch05_multiple_files/vectors.h"

namespace cpp4q::armstrong::ch05 {
namespace {

TEST(VectorsTest, Linspace) {
  const auto v = Linspace(0, 1, 11);
  ASSERT_EQ(v.size(), 11u);
  EXPECT_EQ(v.front(), 0);
  EXPECT_EQ(v.back(), 1);
  EXPECT_NEAR(v[3], 0.3, 1e-15);
  EXPECT_EQ(Linspace(2, 3, 1), std::vector<double>{2});
  EXPECT_THROW(Linspace(0, 1, 0), std::invalid_argument);
}

TEST(VectorsTest, Statistics) {
  const std::vector<double> v{2, 4, 4, 4, 5, 5, 7, 9};
  EXPECT_DOUBLE_EQ(Sum(v), 40);
  EXPECT_DOUBLE_EQ(Mean(v), 5);
  EXPECT_DOUBLE_EQ(StandardDeviation(v, true), 2);
  EXPECT_NEAR(StandardDeviation(v), std::sqrt(32.0 / 7), 1e-15);
  EXPECT_EQ(Min(v), 2);
  EXPECT_EQ(Max(v), 9);
  EXPECT_THROW(Mean(std::vector<double>{}), std::invalid_argument);
  EXPECT_THROW(StandardDeviation(std::vector<double>{1}),
               std::invalid_argument);
}

TEST(VectorsTest, StdIsStableForLargeOffsets) {
  // Sum-of-squares formulas lose everything here; Welford does not.
  const std::vector<double> v{1e9 + 4, 1e9 + 7, 1e9 + 13, 1e9 + 16};
  EXPECT_NEAR(StandardDeviation(v), std::sqrt(30.0), 1e-6);
}

TEST(VectorsTest, PrctileMatchesMatlab) {
  const std::vector<double> v{10, 20, 30, 40};  // at 12.5, 37.5, 62.5, 87.5
  EXPECT_DOUBLE_EQ(Prctile(v, 50), 25);
  EXPECT_DOUBLE_EQ(Prctile(v, 12.5), 10);
  EXPECT_DOUBLE_EQ(Prctile(v, 25), 15);
  EXPECT_DOUBLE_EQ(Prctile(v, 0), 10);
  EXPECT_DOUBLE_EQ(Prctile(v, 100), 40);
  EXPECT_DOUBLE_EQ(Prctile(std::vector<double>{3}, 70), 3);
  EXPECT_THROW(Prctile(v, 101), std::invalid_argument);
}

TEST(VectorsTest, RandomMomentsWithinFourSigma) {
  Rng rng(1);
  constexpr int kN = 200'000;
  const auto u = RandUniform(kN, rng);
  EXPECT_NEAR(Mean(u), 0.5, 4 * std::sqrt(1.0 / 12 / kN));
  EXPECT_GE(Min(u), 0.0);
  EXPECT_LT(Max(u), 1.0);
  const auto z = RandN(kN, rng);
  EXPECT_NEAR(Mean(z), 0.0, 4 / std::sqrt(kN));
  EXPECT_NEAR(StandardDeviation(z), 1.0, 4 * std::sqrt(0.5 / kN));
  EXPECT_NEAR(Prctile(z, 97.5), 1.959964, 0.02);
}

TEST(VectorsTest, SameSeedSameNumbers) {
  Rng a(7), b(7);
  EXPECT_EQ(RandN(100, a), RandN(100, b));
}

}  // namespace
}  // namespace cpp4q::armstrong::ch05
