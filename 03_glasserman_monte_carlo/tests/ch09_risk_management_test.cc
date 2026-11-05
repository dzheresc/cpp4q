#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch09_risk_management/credit.h"
#include "ch09_risk_management/portfolio_var.h"

namespace cpp4q::glasserman::ch09 {
namespace {

using ch01::OptionType;

OptionPortfolio SampleBook() {
  Market m{.spots = {100.0, 50.0},
           .vols = {0.3, 0.25},
           .correlation = {1.0, 0.5, 0.5, 1.0},
           .rate = 0.04};
  return OptionPortfolio(m, {{0, OptionType::kCall, 100.0, 0.5, -10.0},
                             {0, OptionType::kPut, 95.0, 0.5, -10.0},
                             {1, OptionType::kCall, 55.0, 0.25, -20.0}});
}

TEST(QuantileTest, VarAndExpectedShortfall) {
  std::vector<double> losses(1000);
  std::iota(losses.begin(), losses.end(), 1.0);  // 1..1000
  EXPECT_DOUBLE_EQ(ValueAtRisk(losses, 0.01), 990.0);
  EXPECT_DOUBLE_EQ(ExpectedShortfall(losses, 0.01), (990.0 + 1000.0) / 2);
  EXPECT_THROW(ValueAtRisk({}, 0.01), std::invalid_argument);
  EXPECT_THROW(ValueAtRisk(losses, 1.5), std::invalid_argument);
}

TEST(PortfolioTest, ZeroMoveLosesOnlyTheta) {
  const auto book = SampleBook();
  const std::vector<double> spots = book.market().spots;
  EXPECT_NEAR(book.Loss(spots, 0.0), 0.0, 1e-12);
  const auto g = book.Sensitivities();
  const double dt = 1.0 / 252;
  const std::vector<double> zero(2, 0.0);
  EXPECT_NEAR(book.Loss(spots, dt), OptionPortfolio::ApproxLoss(g, zero, dt),
              0.02 * std::abs(g.theta * dt));
}

TEST(PortfolioTest, DeltaGammaMatchesSmallMoves) {
  const auto book = SampleBook();
  const auto g = book.Sensitivities();
  const std::vector<double> ds = {0.5, -0.3};
  std::vector<double> spots = book.market().spots;
  spots[0] += ds[0];
  spots[1] += ds[1];
  EXPECT_NEAR(book.Loss(spots, 0.0), OptionPortfolio::ApproxLoss(g, ds, 0.0),
              1e-3);
}

TEST(PortfolioTest, DeltaGammaVarCloseToFullRevaluation) {
  const auto book = SampleBook();
  const double h = 10.0 / 252;
  const auto full = SimulateLosses(book, h, 50'000, false, 1);
  const auto dg = SimulateLosses(book, h, 50'000, true, 1);
  const double v_full = ValueAtRisk(full, 0.01);
  const double v_dg = ValueAtRisk(dg, 0.01);
  EXPECT_GT(v_full, 0.0);
  EXPECT_NEAR(v_dg, v_full, 0.1 * v_full);
  EXPECT_GE(ExpectedShortfall(full, 0.01), v_full);
}

TEST(JacobiEigenTest, ReconstructsMatrix) {
  const std::vector<double> a = {4, 1, -2, 1, 2, 0.5, -2, 0.5, 3};
  const auto e = JacobiEigen(a, 3);
  for (std::size_t r = 0; r < 3; ++r) {
    for (std::size_t c = 0; c < 3; ++c) {
      double v = 0.0, dot = 0.0;
      for (std::size_t k = 0; k < 3; ++k) {
        v += e.vectors[r * 3 + k] * e.values[k] * e.vectors[c * 3 + k];
        dot += e.vectors[k * 3 + r] * e.vectors[k * 3 + c];
      }
      EXPECT_NEAR(v, a[r * 3 + c], 1e-12);
      EXPECT_NEAR(dot, r == c ? 1.0 : 0.0, 1e-12);
    }
  }
}

TEST(PortfolioTest, DiagonalFormMatchesDeltaGamma) {
  const auto book = SampleBook();
  const double h = 10.0 / 252;
  const auto q = DiagonalizeDeltaGamma(book, h);
  const auto g = book.Sensitivities();
  const auto& a = book.correlation_factor();
  const double y[2] = {0.7, -1.3};
  double z[2] = {0, 0}, ds[2];
  double quad = q.constant;
  for (std::size_t j = 0; j < 2; ++j) {
    quad += q.linear[j] * y[j] + q.quadratic[j] * y[j] * y[j];
    for (std::size_t r = 0; r < 2; ++r) z[r] += q.rotation[r * 2 + j] * y[j];
  }
  for (std::size_t i = 0; i < 2; ++i) {
    double x = 0.0;
    for (std::size_t k = 0; k <= i; ++k) x += a[i * 2 + k] * z[k];
    ds[i] = book.market().spots[i] * book.market().vols[i] * std::sqrt(h) * x;
  }
  EXPECT_NEAR(quad, OptionPortfolio::ApproxLoss(g, ds, h), 1e-9);
}

class TailIsTest : public ::testing::TestWithParam<IsMethod> {};

TEST_P(TailIsTest, UnbiasedOnShortStraddleBook) {
  const auto book = SampleBook();
  const double h = 10.0 / 252;
  const auto losses = SimulateLosses(book, h, 200'000, false, 2);
  const double x = ValueAtRisk(losses, 0.002);  // P(L > x) ~ 0.002
  const auto plain = TailProbability(book, h, x, 200'000, 3);
  const auto is = TailProbabilityIs(book, h, x, 20'000, 3, GetParam());
  EXPECT_NEAR(plain.mean, is.mean,
              4 * std::hypot(plain.std_error, is.std_error));
}

INSTANTIATE_TEST_SUITE_P(Methods, TailIsTest,
                         ::testing::Values(IsMethod::kMeanShift,
                                           IsMethod::kDeltaGammaTwist));

TEST(PortfolioTest, DeltaGammaTwistBeatsPlainAndMeanShiftWhenGammaDominates) {
  const auto book = SampleBook();
  const double h = 10.0 / 252;
  const auto losses = SimulateLosses(book, h, 200'000, false, 4);
  const double x = ValueAtRisk(losses, 0.002);
  const auto plain = TailProbability(book, h, x, 20'000, 5);
  const auto shift =
      TailProbabilityIs(book, h, x, 20'000, 5, IsMethod::kMeanShift);
  const auto twist =
      TailProbabilityIs(book, h, x, 20'000, 5, IsMethod::kDeltaGammaTwist);
  EXPECT_LT(twist.std_error, plain.std_error / 3);
  EXPECT_LT(twist.std_error, shift.std_error);
}

std::vector<Obligor> HomogeneousBook(std::size_t n, double p, double a) {
  return std::vector<Obligor>(n, Obligor{p, 1.0, a});
}

TEST(CreditTest, ConditionalProbabilitiesAverageToUnconditional) {
  const GaussianCopulaPortfolio book(HomogeneousBook(1, 0.02, 0.5));
  // E[p(Y)] = p: integrate over the normal density by the midpoint rule.
  double sum = 0.0;
  for (double y = -10; y < 10; y += 0.001) {
    sum += book.ConditionalDefaultProb(0, y + 0.0005) *
           ch01::NormalPdf(y + 0.0005) * 0.001;
  }
  EXPECT_NEAR(sum, 0.02, 1e-9);
  EXPECT_LT(book.ConditionalDefaultProb(0, 1.0), 0.02);
  EXPECT_GT(book.ConditionalDefaultProb(0, -1.0), 0.02);
}

TEST(CreditTest, ThetaHitsTargetMean) {
  const GaussianCopulaPortfolio book(HomogeneousBook(50, 0.01, 0.3));
  const std::vector<double> p(50, 0.02);
  const double theta = book.SolveTheta(p, 10.0);
  double mean = 0.0;
  for (double pk : p) {
    const double e = std::exp(theta);
    mean += pk * e / (1 + pk * (e - 1));
  }
  EXPECT_NEAR(mean, 10.0, 1e-8);
  EXPECT_EQ(book.SolveTheta(p, 0.5), 0.0);  // mean 1.0 already above 0.5
  // A threshold at or above the total exposure has no finite twist (the
  // first version returned theta = 2^20, and exp(theta c) overflowed).
  EXPECT_THROW(book.SolveTheta(p, 50.0), std::invalid_argument);
}

TEST(CreditTest, ImportanceSamplingMatchesPlainAndIsSharper) {
  std::vector<Obligor> obligors;
  for (int k = 0; k < 100; ++k) {
    obligors.push_back(
        {0.01 * (1 + k % 5), 1.0 + (k % 3), 0.3 + 0.05 * (k % 4)});
  }
  const GaussianCopulaPortfolio book(obligors);
  EXPECT_NEAR(book.ExpectedLoss(), 5.96, 1e-12);
  const double x = 30.0;
  const double shift = book.FindFactorShift(x);
  EXPECT_LT(shift, -1.0);
  const auto plain = book.TailProbability(x, 100'000, 1);
  const auto is = book.TailProbabilityIs(x, shift, 20'000, 2);
  EXPECT_GT(plain.mean, 0.0);
  EXPECT_NEAR(plain.mean, is.mean,
              4 * std::hypot(plain.std_error, is.std_error));
  EXPECT_LT(is.std_error * std::sqrt(20'000.0),
            plain.std_error * std::sqrt(100'000.0) / 5);
}

TEST(CreditTest, RejectsBadObligors) {
  EXPECT_THROW(GaussianCopulaPortfolio({{0.0, 1.0, 0.2}}),
               std::invalid_argument);
  EXPECT_THROW(GaussianCopulaPortfolio({{0.1, 1.0, 1.0}}),
               std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::glasserman::ch09
