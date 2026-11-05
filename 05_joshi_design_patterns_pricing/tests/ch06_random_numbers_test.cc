#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch06_random_numbers/random.h"

namespace cpp4q::joshi::ch06 {
namespace {

TEST(ParkMillerTest, MinimalStandardCheckValue) {
  // Park & Miller (1988): from seed 1, the 10,000th value is 1043618065.
  ParkMiller pm(1);
  std::int64_t x = 0;
  for (int i = 0; i < 10'000; ++i) x = pm.GetOneRandomInteger();
  EXPECT_EQ(x, 1043618065);
}

TEST(ParkMillerTest, DiscardEqualsDrawing) {
  for (const std::uint64_t n : {0ULL, 1ULL, 2ULL, 7ULL, 1000ULL, 123457ULL}) {
    ParkMiller a(12345), b(12345);
    a.Discard(n);
    for (std::uint64_t i = 0; i < n; ++i) b.GetOneRandomInteger();
    EXPECT_EQ(a.GetOneRandomInteger(), b.GetOneRandomInteger()) << n;
  }
  // The full period is m - 1: discarding it returns to the start.
  ParkMiller c(99);
  c.Discard(ParkMiller::kM - 1);
  EXPECT_EQ(c.GetOneRandomInteger(), ParkMiller(99).GetOneRandomInteger());
}

TEST(RandomParkMillerTest, SkipResetAndClone) {
  RandomParkMiller a(3, 42);
  std::vector<double> first(3), v(3), w(3);
  a.GetUniforms(first);
  for (const double u : first) {
    EXPECT_GT(u, 0.0);
    EXPECT_LT(u, 1.0);
  }
  a.Skip(5);
  a.GetUniforms(v);  // vector number 7
  a.Reset();
  for (int i = 0; i < 7; ++i) a.GetUniforms(w);
  EXPECT_EQ(v, w);
  const auto clone = a.Clone();
  a.GetUniforms(v);
  clone->GetUniforms(w);
  EXPECT_EQ(v, w);
  a.SetSeed(42);
  a.GetUniforms(v);
  EXPECT_EQ(v, first);
}

TEST(AntiTheticTest, PairsSumToOne) {
  AntiThetic gen{Wrapper<RandomBase>(RandomParkMiller(4, 1))};
  std::vector<double> u(4), v(4);
  for (int k = 0; k < 10; ++k) {
    gen.GetUniforms(u);
    gen.GetUniforms(v);
    for (std::size_t i = 0; i < 4; ++i) EXPECT_DOUBLE_EQ(u[i] + v[i], 1.0);
  }
  std::vector<double> z(4), z2(4);
  gen.GetGaussians(z);
  gen.GetGaussians(z2);
  for (std::size_t i = 0; i < 4; ++i) EXPECT_NEAR(z[i], -z2[i], 1e-8);
}

TEST(AntiTheticTest, SkipMatchesDrawing) {
  for (std::uint64_t pre = 0; pre < 2; ++pre) {
    for (std::uint64_t skip = 0; skip < 5; ++skip) {
      AntiThetic a{Wrapper<RandomBase>(RandomParkMiller(2, 9))};
      AntiThetic b{Wrapper<RandomBase>(RandomParkMiller(2, 9))};
      std::vector<double> u(2), v(2);
      for (std::uint64_t i = 0; i < pre; ++i) {
        a.GetUniforms(u);
        b.GetUniforms(v);
      }
      a.Skip(skip);
      for (std::uint64_t i = 0; i < skip; ++i) b.GetUniforms(v);
      a.GetUniforms(u);
      b.GetUniforms(v);
      EXPECT_EQ(u, v) << pre << " " << skip;
    }
  }
}

TEST(HaltonTest, RadicalInverseAndSequence) {
  EXPECT_DOUBLE_EQ(RandomHalton::RadicalInverse(1, 2), 0.5);
  EXPECT_DOUBLE_EQ(RandomHalton::RadicalInverse(6, 2), 0.375);  // 110 -> .011
  EXPECT_DOUBLE_EQ(RandomHalton::RadicalInverse(5, 3), 7.0 / 9);  // 12 -> .21
  // The fast paths (bit reversal, 32-bit digits) agree with the generic
  // digit loop.
  const auto slow = [](std::uint64_t n, std::uint32_t b) {
    double inv = 1.0 / b, r = 0.0;
    for (; n > 0; n /= b, inv /= b) r += static_cast<double>(n % b) * inv;
    return r;
  };
  for (std::uint64_t n = 1; n < 5000; n += 7) {
    for (const std::uint32_t b : {2u, 3u, 5u, 311u}) {
      EXPECT_NEAR(RandomHalton::RadicalInverse(n, b), slow(n, b), 1e-15);
    }
  }
  EXPECT_NEAR(RandomHalton::RadicalInverse((1ULL << 40) + 3, 3),
              slow((1ULL << 40) + 3, 3), 1e-15);
  RandomHalton h(2);
  std::vector<double> u(2);
  h.GetUniforms(u);
  EXPECT_DOUBLE_EQ(u[0], 0.5);
  EXPECT_DOUBLE_EQ(u[1], 1.0 / 3);
  h.Skip(4);
  h.GetUniforms(u);  // point 6
  EXPECT_DOUBLE_EQ(u[0], 0.375);
  EXPECT_THROW(RandomHalton(65), std::invalid_argument);
}

TEST(SimpleMc6Test, GeneratorsAgreeWithBlackScholes) {
  const ch04::VanillaOption call(ch04::PayOffCall(100), 1.0);
  const double exact = BlackScholesCall(100, 100, 0.05, 0, 0.2, 1);
  RandomParkMiller pm(1, 3);
  ch05::StatisticsMoments s1;
  SimpleMonteCarlo6(call, 100, 0.2, 0.05, 100'000, s1, pm);
  EXPECT_NEAR(s1.GetResultsSoFar()[0][0], exact,
              4 * s1.GetResultsSoFar()[0][1]);
  // Quasi-random: error (2.4e-3) a twentieth of the pseudo-random
  // standard error (0.046) at the same 100,000 points.
  RandomHalton halton(1);
  ch05::StatisticsMean s2;
  SimpleMonteCarlo6(call, 100, 0.2, 0.05, 100'000, s2, halton);
  EXPECT_NEAR(s2.GetResultsSoFar()[0][0], exact, 4e-3);
  EXPECT_LT(4e-3, 0.1 * s1.GetResultsSoFar()[0][1]);
}

TEST(SimpleMc6Test, AntitheticReducesVariance) {
  // Pairs (z, -z): the variance of pair means, which is what antithetic
  // sampling should shrink, compared to plain pairs.
  const ch04::VanillaOption call(ch04::PayOffCall(100), 1.0);
  const auto pair_variance = [&](RandomBase& gen) {
    ch05::StatisticsMoments pairs;
    std::vector<double> z(1);
    gen.ResetDimensionality(1);
    for (int i = 0; i < 50'000; ++i) {
      double sum = 0;
      for (int k = 0; k < 2; ++k) {
        gen.GetGaussians(z);
        sum += call.OptionPayOff(100 * std::exp(0.03 + 0.2 * z[0]));
      }
      pairs.DumpOneResult(sum / 2);
    }
    return pairs.GetResultsSoFar()[0][2];
  };
  RandomParkMiller plain(1, 5);
  AntiThetic anti{Wrapper<RandomBase>(RandomParkMiller(1, 5))};
  EXPECT_LT(pair_variance(anti), 0.5 * pair_variance(plain));
}

}  // namespace
}  // namespace cpp4q::joshi::ch06
