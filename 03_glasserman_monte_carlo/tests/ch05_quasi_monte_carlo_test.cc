#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "ch02_random_numbers/rng.h"
#include "ch05_quasi_monte_carlo/low_discrepancy.h"
#include "ch05_quasi_monte_carlo/qmc_pricing.h"

namespace cpp4q::glasserman::ch05 {
namespace {

TEST(RadicalInverseTest, VanDerCorput) {
  EXPECT_DOUBLE_EQ(RadicalInverse(1, 2), 0.5);
  EXPECT_DOUBLE_EQ(RadicalInverse(2, 2), 0.25);
  EXPECT_DOUBLE_EQ(RadicalInverse(3, 2), 0.75);
  EXPECT_DOUBLE_EQ(RadicalInverse(6, 2), 0.375);
  EXPECT_NEAR(RadicalInverse(5, 3), 7.0 / 9.0, 1e-15);  // 5 = 12_3
  EXPECT_EQ(RadicalInverse(0, 7), 0.0);
}

TEST(HaltonTest, UsesPrimeBases) {
  EXPECT_EQ(FirstPrimes(6), (std::vector<std::uint32_t>{2, 3, 5, 7, 11, 13}));
  Halton h(3);
  double p[3];
  h.Next(p);
  EXPECT_DOUBLE_EQ(p[0], 0.5);
  EXPECT_NEAR(p[1], 1.0 / 3, 1e-15);
  EXPECT_NEAR(p[2], 0.2, 1e-15);
}

// x^s + a_1 x^(s-1) + ... + 1 must be primitive over GF(2): the order of
// x modulo it must be 2^s - 1.
TEST(SobolTest, PolynomialsArePrimitive) {
  for (const auto& p : SobolPolynomials()) {
    const unsigned s = p.degree;
    const std::uint32_t poly = (1u << s) | (p.a << 1) | 1u;
    std::uint32_t x = 1;  // x^0
    std::uint32_t order = 0;
    do {
      x <<= 1;
      if (x & (1u << s)) x ^= poly;
      ++order;
    } while (x != 1 && order <= (1u << s));
    EXPECT_EQ(order, (1u << s) - 1) << "degree " << s << " a " << p.a;
    for (unsigned i = 0; i < s; ++i) {
      EXPECT_EQ(p.m[i] % 2, 1u);
      EXPECT_LT(p.m[i], 1u << (i + 1));
    }
  }
}

TEST(SobolTest, FirstPointsOfFirstDimensions) {
  Sobol sobol(3);
  double p[3];
  sobol.Next(p);  // point 1
  EXPECT_NEAR(p[0], 0.5, 1e-9);
  EXPECT_NEAR(p[1], 0.5, 1e-9);
  EXPECT_NEAR(p[2], 0.5, 1e-9);
  sobol.Next(p);  // point 2
  EXPECT_NEAR(p[0], 0.75, 1e-9);
  EXPECT_NEAR(p[1], 0.25, 1e-9);
  sobol.Next(p);  // point 3
  EXPECT_NEAR(p[0], 0.25, 1e-9);
  EXPECT_NEAR(p[1], 0.75, 1e-9);
}

// Points 0..2^k-1 of every coordinate hit each interval [j/2^k, (j+1)/2^k)
// exactly once. Point 0 is the origin; the sequence starts at point 1, so
// take points 1..2^k-1 plus the origin.
TEST(SobolTest, EachCoordinateIsStratified) {
  constexpr int kLog2 = 10;
  constexpr std::size_t kN = 1u << kLog2;
  Sobol sobol(Sobol::kMaxDim);
  std::vector<std::vector<int>> hits(Sobol::kMaxDim, std::vector<int>(kN, 0));
  for (auto& h : hits) h[0] = 1;  // the origin
  std::vector<double> p(Sobol::kMaxDim);
  for (std::size_t i = 1; i < kN; ++i) {
    sobol.Next(p);
    for (std::size_t d = 0; d < p.size(); ++d) {
      ++hits[d][static_cast<std::size_t>(p[d] * kN)];
    }
  }
  for (std::size_t d = 0; d < hits.size(); ++d) {
    EXPECT_TRUE(std::all_of(hits[d].begin(), hits[d].end(),
                            [](int c) { return c == 1; }))
        << "dimension " << d;
  }
}

TEST(SobolTest, TwoDimensionalProjectionsAreNets) {
  // Points 0..255 of dimensions (1, 2): every 16 x 16 box holds one point.
  Sobol sobol(2);
  std::vector<int> boxes(256, 0);
  boxes[0] = 1;
  double p[2];
  for (int i = 1; i < 256; ++i) {
    sobol.Next(p);
    ++boxes[static_cast<std::size_t>(p[0] * 16) * 16 +
            static_cast<std::size_t>(p[1] * 16)];
  }
  EXPECT_TRUE(
      std::all_of(boxes.begin(), boxes.end(), [](int c) { return c == 1; }));
}

TEST(SobolTest, SeekMatchesSequentialGeneration) {
  Sobol a(5), b(5);
  std::vector<double> pa(5), pb(5);
  for (int i = 0; i < 1000; ++i) a.Next(pa);
  b.Seek(1000);
  b.Next(pb);
  a.Seek(1000);
  a.Next(pa);
  EXPECT_EQ(pa, pb);
  Sobol c(5);
  for (int i = 0; i < 1000; ++i) c.Next(pa);
  EXPECT_EQ(pa, pb);
  EXPECT_THROW(c.Seek(0), std::invalid_argument);
}

TEST(SobolTest, DigitalShiftKeepsStratification) {
  Sobol sobol(4, 12345);
  constexpr std::size_t kN = 256;
  std::vector<std::vector<int>> hits(4, std::vector<int>(kN, 0));
  std::vector<double> p(4);
  // A digital shift permutes the strata: points 1..255 still fall in
  // distinct strata (the shifted origin takes the remaining one).
  for (std::size_t i = 1; i < kN; ++i) {
    sobol.Next(p);
    for (std::size_t d = 0; d < 4; ++d)
      ++hits[d][static_cast<std::size_t>(p[d] * kN)];
  }
  for (std::size_t d = 0; d < 4; ++d) {
    EXPECT_EQ(std::count(hits[d].begin(), hits[d].end(), 1), 255) << d;
  }
}

TEST(SobolTest, RejectsBadDimension) {
  EXPECT_THROW(Sobol(0), std::invalid_argument);
  EXPECT_THROW(Sobol(17), std::invalid_argument);
  Sobol s(2);
  std::vector<double> wrong(3);
  EXPECT_THROW(s.Next(wrong), std::invalid_argument);
}

TEST(DiscrepancyTest, SobolBeatsRandomPoints) {
  constexpr std::size_t kDim = 3, kN = 512;
  std::vector<double> rnd(kN * kDim), sob(kN * kDim);
  ch02::Mrg32k3a rng(1);
  Sobol sobol(kDim);
  for (std::size_t i = 0; i < kN; ++i) {
    for (std::size_t k = 0; k < kDim; ++k)
      rnd[i * kDim + k] = rng.NextUniform();
    sobol.Next(std::span(sob).subspan(i * kDim, kDim));
  }
  EXPECT_LT(L2StarDiscrepancy(sob, kDim), 0.5 * L2StarDiscrepancy(rnd, kDim));
}

TEST(DiscrepancyTest, SinglePointClosedForm) {
  // n = 1, d = 1, x: T^2 = 1/3 - (1 - x^2) + (1 - x).
  const double x[1] = {0.5};
  EXPECT_NEAR(L2StarDiscrepancy(x, 1), std::sqrt(1.0 / 3 - 0.75 + 0.5), 1e-15);
}

TEST(QmcPricingTest, RqmcAgreesWithMcAndIsMoreAccurate) {
  const QmcAsianSpec spec{};
  const auto mc = McAsianCall(spec, 2048 * 16, 1);
  const auto rw =
      RqmcAsianCall(spec, 2048, 16, PathConstruction::kRandomWalk, 1);
  const auto bb =
      RqmcAsianCall(spec, 2048, 16, PathConstruction::kBrownianBridge, 1);
  EXPECT_NEAR(rw.mean, bb.mean, 4 * std::hypot(rw.std_error, bb.std_error));
  EXPECT_NEAR(mc.mean, bb.mean, 4 * std::hypot(mc.std_error, bb.std_error));
  EXPECT_LT(rw.std_error, mc.std_error / 3);
  EXPECT_LT(bb.std_error, rw.std_error);
}

TEST(QmcPricingTest, ConvergesTowardsRqmcReference) {
  const QmcAsianSpec spec{};
  const auto ref =
      RqmcAsianCall(spec, 8192, 16, PathConstruction::kBrownianBridge, 2);
  const double coarse =
      QmcAsianCall(spec, 256, PathConstruction::kBrownianBridge);
  const double fine =
      QmcAsianCall(spec, 16384, PathConstruction::kBrownianBridge);
  EXPECT_LT(std::abs(fine - ref.mean), std::abs(coarse - ref.mean) + 1e-3);
  EXPECT_NEAR(fine, ref.mean, 0.01);
  EXPECT_THROW(QmcAsianCall({{}, 17}, 16, PathConstruction::kRandomWalk),
               std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::glasserman::ch05
