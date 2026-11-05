#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/normal.h"
#include "ch03_sample_paths/brownian.h"
#include "ch03_sample_paths/gbm.h"
#include "ch03_sample_paths/jump_diffusion.h"
#include "ch03_sample_paths/short_rate.h"

namespace cpp4q::glasserman::ch03 {
namespace {

using ch02::Mrg32k3a;
using ch02::Sampler;

TEST(GridTest, UniformGrid) {
  const auto t = UniformGrid(2.0, 4);
  ASSERT_EQ(t.size(), 4u);
  EXPECT_DOUBLE_EQ(t[0], 0.5);
  EXPECT_DOUBLE_EQ(t[3], 2.0);
  EXPECT_THROW(UniformGrid(1.0, 0), std::invalid_argument);
}

// Both constructions must give Cov(W(s), W(t)) = min(s, t).
void CheckCovariance(bool bridge, const std::vector<double>& times) {
  const std::size_t n = times.size();
  const BrownianBridge bb(times);
  Sampler s(Mrg32k3a(1));
  std::vector<double> z(n), w(n), cov(n * n, 0.0);
  constexpr int kPaths = 100'000;
  for (int p = 0; p < kPaths; ++p) {
    s.FillNormal(z);
    if (bridge) {
      bb.Build(z, w);
    } else {
      RandomWalkPath(times, z, w);
    }
    for (std::size_t i = 0; i < n; ++i) {
      for (std::size_t j = 0; j < n; ++j) cov[i * n + j] += w[i] * w[j];
    }
  }
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      const double expected = std::min(times[i], times[j]);
      EXPECT_NEAR(cov[i * n + j] / kPaths, expected, 0.02 * times.back())
          << i << "," << j;
    }
  }
}

TEST(BrownianTest, RandomWalkCovariance) {
  CheckCovariance(false, {0.1, 0.25, 0.5, 0.6, 1.0});
}

TEST(BrownianTest, BridgeCovarianceUniformGrid) {
  CheckCovariance(true, UniformGrid(1.0, 8));
}

TEST(BrownianTest, BridgeCovarianceIrregularOddGrid) {
  CheckCovariance(true, {0.05, 0.3, 0.31, 0.7, 0.9, 1.4, 2.0});
}

TEST(BrownianTest, BridgeFirstNormalSetsTerminalValue) {
  const auto t = UniformGrid(4.0, 16);
  const BrownianBridge bb(t);
  std::vector<double> z(16, 0.0), w(16);
  z[0] = 1.5;
  bb.Build(z, w);
  EXPECT_DOUBLE_EQ(w[15], 1.5 * 2.0);  // sqrt(T) z0
  // With the other normals zero the path is the straight line.
  for (std::size_t i = 0; i < 16; ++i)
    EXPECT_NEAR(w[i], 3.0 * t[i] / 4.0, 1e-14);
}

TEST(BrownianTest, SinglePointBridge) {
  const BrownianBridge bb({2.0});
  double z[1] = {0.5}, w[1];
  bb.Build(z, w);
  EXPECT_DOUBLE_EQ(w[0], 0.5 * std::sqrt(2.0));
}

TEST(BrownianTest, RejectsBadGrids) {
  EXPECT_THROW(BrownianBridge({}), std::invalid_argument);
  EXPECT_THROW(BrownianBridge({0.5, 0.5}), std::invalid_argument);
  EXPECT_THROW(BrownianBridge({0.0, 1.0}), std::invalid_argument);
}

TEST(GbmTest, PathAndBrownianConstructionsAgree) {
  const auto t = UniformGrid(1.0, 5);
  const std::vector<double> z = {0.3, -1.2, 0.8, 0.0, 2.1};
  std::vector<double> w(5), a(5), b(5);
  RandomWalkPath(t, z, w);
  const GbmParams p{100.0, 0.03, 0.25};
  GbmPath(p, t, z, a);
  GbmFromBrownian(p, t, w, b);
  for (std::size_t i = 0; i < 5; ++i) EXPECT_NEAR(a[i], b[i], 1e-10);
}

TEST(GbmTest, MartingaleUnderRiskNeutralDrift) {
  const GbmParams p{100.0, 0.05, 0.3};
  const auto t = UniformGrid(2.0, 4);
  Sampler s(Mrg32k3a(2));
  std::vector<double> z(4), path(4);
  ch01::RunningStats st;
  for (int i = 0; i < 200'000; ++i) {
    s.FillNormal(z);
    GbmPath(p, t, z, path);
    st.Add(path[3] * std::exp(-0.05 * 2.0));
  }
  EXPECT_NEAR(st.Mean(), 100.0, 4 * st.StdError());
}

TEST(MultiAssetGbmTest, LogReturnCorrelation) {
  const std::vector<double> corr = {1.0, 0.7, 0.7, 1.0};
  const MultiAssetGbm m({{100, 0.0, 0.2}, {50, 0.0, 0.4}}, corr);
  Sampler s(Mrg32k3a(3));
  double z[2], out[2];
  double sxy = 0, sxx = 0, syy = 0;
  for (int i = 0; i < 100'000; ++i) {
    s.FillNormal(z);
    m.Terminal(1.0, z, out);
    const double x = std::log(out[0] / 100) + 0.02;
    const double y = std::log(out[1] / 50) + 0.08;
    sxy += x * y;
    sxx += x * x;
    syy += y * y;
  }
  EXPECT_NEAR(sxy / std::sqrt(sxx * syy), 0.7, 0.01);
  EXPECT_NEAR(sxx / 100'000, 0.04, 0.001);
}

TEST(MultiAssetGbmTest, PathEndsAtTerminalForOneStep) {
  const std::vector<double> corr = {1.0, 0.3, 0.3, 1.0};
  const MultiAssetGbm m({{100, 0.05, 0.2}, {50, 0.02, 0.4}}, corr);
  const double z[2] = {0.4, -0.9};
  double a[2], b[2];
  m.Terminal(0.75, z, a);
  const double t[1] = {0.75};
  m.Path(t, z, b);
  EXPECT_NEAR(a[0], b[0], 1e-12);
  EXPECT_NEAR(a[1], b[1], 1e-12);
}

TEST(VasicekTest, ExactSimulationIsUnbiased) {
  const ShortRateParams p{.kappa = 0.3, .theta = 0.05, .sigma = 0.02};
  Sampler s(Mrg32k3a(4));
  // Coarse and fine grids must agree: no discretization error.
  for (std::size_t steps : {1u, 10u}) {
    const auto e = VasicekBondMc(p, 0.03, 5.0, steps, 100'000, s);
    EXPECT_NEAR(e.mean, VasicekBondPrice(p, 0.03, 5.0), 4 * e.std_error)
        << steps;
  }
}

TEST(VasicekTest, StepMoments) {
  const ShortRateParams p{.kappa = 0.8, .theta = 0.04, .sigma = 0.03};
  const VasicekExactStep step(p, 0.5);
  Sampler s(Mrg32k3a(5));
  ch01::RunningStats r, y;
  for (int i = 0; i < 200'000; ++i) {
    const double z1 = s.Normal();
    const auto v = step.Step(0.02, z1, s.Normal());
    r.Add(v.rate);
    y.Add(v.integral);
  }
  const double decay = std::exp(-0.4);
  EXPECT_NEAR(r.Mean(), 0.04 + (0.02 - 0.04) * decay, 1e-4);
  EXPECT_NEAR(r.Variance(), 0.0009 / 1.6 * (1 - decay * decay), 2e-6);
  EXPECT_NEAR(y.Mean(), 0.04 * 0.5 + (0.02 - 0.04) * (1 - decay) / 0.8, 5e-5);
}

TEST(CirTest, ExactStepMean) {
  const ShortRateParams p{.kappa = 0.5, .theta = 0.05, .sigma = 0.15};
  Sampler s(Mrg32k3a(6));
  ch01::RunningStats st;
  for (int i = 0; i < 200'000; ++i) st.Add(CirExactStep(p, 0.02, 1.0, s));
  const double decay = std::exp(-0.5);
  EXPECT_NEAR(st.Mean(), 0.05 + (0.02 - 0.05) * decay, 4 * st.StdError());
}

TEST(CirTest, BondPrice) {
  const ShortRateParams p{.kappa = 0.3, .theta = 0.05, .sigma = 0.1};
  Sampler s(Mrg32k3a(7));
  const auto e = CirBondMc(p, 0.03, 5.0, 50, 40'000, s);
  // Trapezoid bias at 50 steps is far below the noise.
  EXPECT_NEAR(e.mean, CirBondPrice(p, 0.03, 5.0), 4 * e.std_error);
  EXPECT_NEAR(CirBondPrice(p, 0.03, 1e-9), 1.0, 1e-9);
}

TEST(MertonTest, NoJumpsIsBlackScholes) {
  MertonParams p{};
  p.jump_intensity = 0.0;
  const ch01::BlackScholesInputs in{};
  EXPECT_NEAR(MertonCallPrice(p, 100.0, 1.0),
              ch01::BlackScholesPrice(ch01::OptionType::kCall, in), 1e-12);
}

TEST(MertonTest, MonteCarloMatchesSeries) {
  const MertonParams p{};
  Sampler s(Mrg32k3a(8));
  for (double k : {80.0, 100.0, 120.0}) {
    const auto e = MertonCallMc(p, k, 1.0, 200'000, s);
    EXPECT_NEAR(e.mean, MertonCallPrice(p, k, 1.0), 4 * e.std_error) << k;
  }
}

TEST(MertonTest, PathIsMartingale) {
  const MertonParams p{.jump_intensity = 3.0};
  const auto t = UniformGrid(1.0, 4);
  Sampler s(Mrg32k3a(9));
  std::vector<double> path(4);
  ch01::RunningStats st;
  for (int i = 0; i < 100'000; ++i) {
    MertonPath(p, t, s, path);
    st.Add(path[3] * std::exp(-p.rate));
  }
  EXPECT_NEAR(st.Mean(), p.spot, 4 * st.StdError());
}

}  // namespace
}  // namespace cpp4q::glasserman::ch03
