#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ch05_multiple_files/vectors.h"
#include "ch09_monte_carlo/monte_carlo.h"

namespace cpp4q::armstrong::ch09 {
namespace {

TEST(PathTest, ShapeAndPositivity) {
  Rng rng(1);
  const BlackScholesModel m;
  const auto path = GeneratePricePath(m, 1, 12, Measure::kRiskNeutral, rng);
  ASSERT_EQ(path.size(), 12u);
  for (double s : path) EXPECT_GT(s, 0);
  EXPECT_THROW(GeneratePricePath(m, 1, 0, Measure::kRiskNeutral, rng),
               std::invalid_argument);
  EXPECT_THROW(GeneratePricePath(m, 0, 5, Measure::kRiskNeutral, rng),
               std::invalid_argument);
}

TEST(PathTest, TerminalMomentsInEachMeasure) {
  Rng rng(2);
  const BlackScholesModel m;  // drift 10%, r 5%
  constexpr int kN = 100'000;
  for (const auto& [measure, mu] :
       {std::pair{Measure::kRealWorld, m.drift},
        std::pair{Measure::kRiskNeutral, m.risk_free_rate}}) {
    std::vector<double> log_s(kN);
    for (double& x : log_s) {
      x = std::log(GeneratePricePath(m, 2, 4, measure, rng).back());
    }
    const double sd = m.volatility * std::sqrt(2.0);
    EXPECT_NEAR(ch05::Mean(log_s),
                std::log(100) + (mu - 0.5 * 0.04) * 2, 4 * sd / std::sqrt(kN));
    EXPECT_NEAR(ch05::StandardDeviation(log_s), sd,
                4 * sd * std::sqrt(0.5 / kN));
  }
}

TEST(MonteCarloTest, CallAndPutWithinFourSigma) {
  Rng rng(3);
  BlackScholesModel m;
  m.date = 0.25;
  const MonteCarloPricer pricer(200'000);
  for (double k : {80.0, 100.0, 120.0}) {
    const ch08::CallOption c(k, 1.25);
    const ch08::PutOption p(k, 1.25);
    const Estimate ec = pricer.Price(c, m, rng);
    const Estimate ep = pricer.Price(p, m, rng);
    EXPECT_TRUE(ec.Contains(c.Price(m))) << k << " " << ec.price;
    EXPECT_TRUE(ep.Contains(p.Price(m))) << k << " " << ep.price;
    EXPECT_LT(ec.std_error, 0.05);
  }
}

TEST(MonteCarloTest, StdErrorShrinksLikeRootN) {
  const BlackScholesModel m;
  const ch08::CallOption c(100, 1);
  Rng a(4), b(4);
  const double small = MonteCarloPricer(10'000).Price(c, m, a).std_error;
  const double big = MonteCarloPricer(160'000).Price(c, m, b).std_error;
  EXPECT_NEAR(small / big, 4.0, 0.2);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch09
