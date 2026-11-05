#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/distributions.h"
#include "ch02_random_numbers/normal.h"
#include "ch02_random_numbers/parallel_mc.h"
#include "ch02_random_numbers/rng.h"

namespace cpp4q::glasserman::ch02 {
namespace {

static_assert(std::uniform_random_bit_generator<Mrg32k3a>);

TEST(Mrg32k3aTest, ReferenceSeedOutputs) {
  // Recurrence evaluated independently (Python big integers).
  Mrg32k3a rng;
  EXPECT_EQ(rng(), 545508589u);
  EXPECT_EQ(rng(), 1368065410u);
  EXPECT_EQ(rng(), 1327943761u);
}

TEST(Mrg32k3aTest, UniformsInOpenIntervalWithRightMoments) {
  Mrg32k3a rng(1);
  ch01::RunningStats st;
  for (int i = 0; i < 200'000; ++i) {
    const double u = rng.NextUniform();
    ASSERT_GT(u, 0.0);
    ASSERT_LT(u, 1.0);
    st.Add(u);
  }
  EXPECT_NEAR(st.Mean(), 0.5, 0.003);
  EXPECT_NEAR(st.Variance(), 1.0 / 12, 0.001);
}

TEST(Mrg32k3aTest, AdvanceMatchesStepping) {
  for (std::uint64_t n : {0ull, 1ull, 2ull, 3ull, 17ull, 1000ull, 65537ull}) {
    Mrg32k3a a(42), b(42);
    for (std::uint64_t i = 0; i < n; ++i) a();
    b.Advance(n);
    EXPECT_EQ(a, b) << n;
    EXPECT_EQ(a(), b());
  }
}

TEST(Mrg32k3aTest, SubstreamsComposeAndDiffer) {
  const Mrg32k3a base(7);
  Mrg32k3a stepped = base;
  for (int i = 0; i < 5; ++i) stepped.NextSubstream();
  EXPECT_EQ(stepped, base.Substream(5));
  EXPECT_EQ(base.Substream(2).Substream(3), base.Substream(5));
  Mrg32k3a s0 = base, s1 = base.Substream(1);
  EXPECT_NE(s0(), s1());
}

TEST(Mrg32k3aTest, SeedsGiveDifferentStreams) {
  Mrg32k3a a(1), b(2);
  EXPECT_NE(a(), b());
}

TEST(LcgTest, MinStdKnownValue) {
  // Park and Miller: seed 1 gives 1043618065 after 10000 steps.
  MinStd g(1);
  for (int i = 0; i < 10'000; ++i) g.NextUniform();
  EXPECT_EQ(g.state(), 1043618065u);
}

TEST(LcgTest, RanduTriplesLieOnPlanes) {
  Randu g(12345);
  std::int64_t x0 = g.NextInt(), x1 = g.NextInt();
  for (int i = 0; i < 1000; ++i) {
    const std::int64_t x2 = g.NextInt();
    const std::int64_t m = 1LL << 31;
    EXPECT_EQ(((x2 - 6 * x1 + 9 * x0) % m + m) % m, 0);
    x0 = x1;
    x1 = x2;
  }
}

TEST(InverseNormalTest, RoundTripsThroughCdf) {
  for (double u = 1e-10; u < 1.0; u = u < 0.01 ? u * 10 : u + 0.01) {
    EXPECT_NEAR(InverseNormalCdf(u), InverseNormalCdfAccurate(u),
                3e-9 * (1 + std::abs(InverseNormalCdfAccurate(u))))
        << u;
    EXPECT_NEAR(ch01::NormalCdf(InverseNormalCdfAccurate(u)), u,
                1e-14 * (1 + u))
        << u;
  }
  EXPECT_NEAR(InverseNormalCdfAccurate(0.975), 1.959963984540054, 1e-13);
  EXPECT_NEAR(InverseNormalCdfAccurate(1e-12), -7.0344838, 1e-6);
}

TEST(InverseNormalTest, MonotoneAndOdd) {
  double prev = -1e300;
  for (int i = 1; i < 10'000; ++i) {
    const double u = i / 10'000.0;
    const double x = InverseNormalCdf(u);
    EXPECT_GT(x, prev);
    EXPECT_NEAR(x, -InverseNormalCdf(1.0 - u), 1e-9);
    prev = x;
  }
}

class NormalMethodTest : public ::testing::TestWithParam<NormalMethod> {};

TEST_P(NormalMethodTest, MomentsAndTail) {
  Sampler s(Mrg32k3a(3), GetParam());
  ch01::RunningStats st, fourth;
  int beyond = 0;
  constexpr int kN = 400'000;
  for (int i = 0; i < kN; ++i) {
    const double z = s.Normal();
    st.Add(z);
    fourth.Add(z * z * z * z);
    if (z > 1.959963984540054) ++beyond;
  }
  EXPECT_NEAR(st.Mean(), 0.0, 0.006);
  EXPECT_NEAR(st.Variance(), 1.0, 0.008);
  EXPECT_NEAR(fourth.Mean(), 3.0, 0.05);
  EXPECT_NEAR(beyond / static_cast<double>(kN), 0.025, 0.001);
}

INSTANTIATE_TEST_SUITE_P(All, NormalMethodTest,
                         ::testing::Values(NormalMethod::kInversion,
                                           NormalMethod::kBoxMuller,
                                           NormalMethod::kMarsagliaPolar));

TEST(DistributionsTest, ExponentialAndPoisson) {
  Sampler s(Mrg32k3a(4));
  ch01::RunningStats e, p, big;
  for (int i = 0; i < 100'000; ++i) {
    e.Add(SampleExponential(s, 2.0));
    p.Add(static_cast<double>(SamplePoisson(s, 3.5)));
  }
  for (int i = 0; i < 20'000; ++i) {
    big.Add(static_cast<double>(SamplePoisson(s, 850.0)));
  }
  EXPECT_NEAR(e.Mean(), 0.5, 0.005);
  EXPECT_NEAR(p.Mean(), 3.5, 0.03);
  EXPECT_NEAR(p.Variance(), 3.5, 0.06);
  EXPECT_NEAR(big.Mean(), 850.0, 1.0);
  EXPECT_NEAR(big.Variance(), 850.0, 30.0);
  EXPECT_EQ(SamplePoisson(s, 0.0), 0u);
  EXPECT_THROW(SamplePoisson(s, -1.0), std::invalid_argument);
}

TEST(DistributionsTest, GammaAcceptanceRejection) {
  Sampler s(Mrg32k3a(5));
  for (double shape : {0.4, 1.0, 2.5, 30.0}) {
    ch01::RunningStats st;
    for (int i = 0; i < 100'000; ++i) st.Add(SampleGamma(s, shape, 2.0));
    EXPECT_NEAR(st.Mean(), 2.0 * shape, 0.02 * shape + 0.01) << shape;
    EXPECT_NEAR(st.Variance(), 4.0 * shape, 0.06 * shape + 0.03) << shape;
  }
}

TEST(DistributionsTest, NoncentralChiSquareMoments) {
  Sampler s(Mrg32k3a(6));
  ch01::RunningStats st;
  for (int i = 0; i < 200'000; ++i) {
    st.Add(SampleNoncentralChiSquare(s, 1.5, 5.0));
  }
  EXPECT_NEAR(st.Mean(), 6.5, 0.04);                  // d + lambda
  EXPECT_NEAR(st.Variance(), 2 * (1.5 + 10.0), 0.4);  // 2 (d + 2 lambda)
}

TEST(CholeskyTest, FactorReproducesMatrix) {
  const std::vector<double> a = {4, 2, 0.4, 2, 5, 1, 0.4, 1, 3};
  const auto l = CholeskyFactor(a, 3);
  for (std::size_t i = 0; i < 3; ++i) {
    for (std::size_t j = 0; j < 3; ++j) {
      double v = 0;
      for (std::size_t k = 0; k < 3; ++k) v += l[i * 3 + k] * l[j * 3 + k];
      EXPECT_NEAR(v, a[i * 3 + j], 1e-12);
    }
    for (std::size_t j = i + 1; j < 3; ++j) EXPECT_EQ(l[i * 3 + j], 0.0);
  }
  EXPECT_THROW(CholeskyFactor(std::vector<double>{1, 2, 2, 1}, 2),
               std::invalid_argument);
  EXPECT_THROW(CholeskyFactor(std::vector<double>{1, 0.5, 0.4, 1}, 2),
               std::invalid_argument);
}

TEST(MultivariateNormalTest, SampleCovariance) {
  const std::vector<double> cov = {1.0, 0.6, 0.6, 2.0};
  MultivariateNormal mvn({1.0, -1.0}, cov);
  Sampler s(Mrg32k3a(8));
  double out[2];
  ch01::RunningStats x, y;
  double sxy = 0.0;
  constexpr int kN = 200'000;
  for (int i = 0; i < kN; ++i) {
    mvn.Sample(s, out);
    x.Add(out[0]);
    y.Add(out[1]);
    sxy += (out[0] - 1.0) * (out[1] + 1.0);
  }
  EXPECT_NEAR(x.Mean(), 1.0, 0.01);
  EXPECT_NEAR(y.Mean(), -1.0, 0.01);
  EXPECT_NEAR(y.Variance(), 2.0, 0.03);
  EXPECT_NEAR(sxy / kN, 0.6, 0.015);
}

TEST(ParallelTest, ResultIndependentOfThreadCount) {
  auto path = [](Sampler& s) { return std::exp(0.2 * s.Normal()); };
  const auto one =
      ParallelSimulate(path, 50'000, {.threads = 1, .block_size = 1000});
  const auto four =
      ParallelSimulate(path, 50'000, {.threads = 4, .block_size = 1000});
  EXPECT_EQ(one.count(), 50'000u);
  EXPECT_DOUBLE_EQ(one.Mean(), four.Mean());
  EXPECT_DOUBLE_EQ(one.Variance(), four.Variance());
  EXPECT_NEAR(one.Mean(), std::exp(0.02), 4 * one.StdError());
}

TEST(ParallelTest, PricesCall) {
  const ch01::BlackScholesInputs in{};
  auto path = [&](Sampler& s) {
    const double st = in.spot * std::exp(in.rate - 0.02 + 0.2 * s.Normal());
    return std::exp(-in.rate) * std::max(st - in.strike, 0.0);
  };
  const auto st = ParallelSimulate(path, 200'000, {.threads = 3, .seed = 9});
  EXPECT_NEAR(st.Mean(), ch01::BlackScholesPrice(ch01::OptionType::kCall, in),
              4 * st.StdError());
}

TEST(ParallelTest, PropagatesExceptions) {
  std::atomic<int> calls{0};
  auto path = [&](Sampler&) -> double {
    if (++calls == 777) throw std::runtime_error("bad path");
    return 1.0;
  };
  EXPECT_THROW(
      ParallelSimulate(path, 100'000, {.threads = 4, .block_size = 100}),
      std::runtime_error);
}

TEST(ParallelTest, ZeroPaths) {
  const auto st = ParallelSimulate([](Sampler&) { return 1.0; }, 0);
  EXPECT_EQ(st.count(), 0u);
}

}  // namespace
}  // namespace cpp4q::glasserman::ch02
