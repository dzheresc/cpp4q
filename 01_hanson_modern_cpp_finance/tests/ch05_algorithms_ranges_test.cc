#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch05_algorithms_ranges/return_stats.h"

namespace cpp4q::hanson::ch05 {
namespace {

TEST(ReturnsTest, SimpleAndLog) {
  const std::vector<double> p{100.0, 110.0, 99.0};
  const auto r = SimpleReturns(p);
  ASSERT_EQ(r.size(), 2u);
  EXPECT_NEAR(r[0], 0.10, 1e-15);
  EXPECT_NEAR(r[1], -0.10, 1e-15);
  const auto lr = LogReturns(p);
  EXPECT_NEAR(lr[0] + lr[1], std::log(0.99), 1e-15);
  EXPECT_TRUE(SimpleReturns(std::vector<double>{1.0}).empty());
}

TEST(MomentsTest, MeanVarianceSharpe) {
  const std::vector<double> x{2, 4, 4, 4, 5, 5, 7, 9};
  EXPECT_DOUBLE_EQ(Mean(x), 5.0);
  EXPECT_DOUBLE_EQ(SampleVariance(x), 32.0 / 7.0);
  EXPECT_DOUBLE_EQ(SampleVariance(std::vector<double>{1.0}), 0.0);
  EXPECT_NEAR(SharpeRatio(x, 1.0, 4.0), 4.0 / std::sqrt(32.0 / 7.0) * 2.0,
              1e-12);
  EXPECT_EQ(SharpeRatio(std::vector<double>{1, 1, 1}, 0.0, 252.0), 0.0);
}

TEST(DrawdownTest, PeakToTrough) {
  EXPECT_DOUBLE_EQ(MaxDrawdown(std::vector<double>{100, 120, 90, 130, 117}),
                   0.25);
  EXPECT_DOUBLE_EQ(MaxDrawdown(std::vector<double>{1, 2, 3}), 0.0);
}

TEST(MovingAverageTest, Windows) {
  const std::vector<double> p{1, 2, 3, 4, 5};
  EXPECT_EQ(MovingAverage(p, 2), (std::vector<double>{1.5, 2.5, 3.5, 4.5}));
  EXPECT_EQ(MovingAverage(p, 5), (std::vector<double>{3.0}));
  EXPECT_TRUE(MovingAverage(p, 6).empty());
  EXPECT_THROW((void)MovingAverage(p, 0), std::invalid_argument);
}

TEST(MovingAverageTest, SlidingMatchesRangesReference) {
  std::vector<double> p;
  for (int i = 0; i < 5000; ++i) p.push_back(100.0 + std::sin(0.01 * i) * 7.0);
  for (std::size_t n : {1u, 3u, 20u, 250u, 4999u}) {
    const auto fast = MovingAverage(p, n);
    const auto ref = MovingAverageRanges(p, n);
    ASSERT_EQ(fast.size(), ref.size());
    for (std::size_t i = 0; i < ref.size(); ++i) {
      ASSERT_NEAR(fast[i], ref[i], 1e-11) << "n=" << n << " i=" << i;
    }
  }
}

TEST(VarTest, EmpiricalQuantile) {
  std::vector<double> r;
  for (int i = 1; i <= 100; ++i) r.push_back(-0.001 * i);  // -0.1 .. -0.001
  EXPECT_NEAR(HistoricalVar(r, 0.95), 0.095, 1e-12);
  EXPECT_THROW((void)HistoricalVar(r, 1.0), std::invalid_argument);
}

TEST(PortfolioTest, InnerProductAndTopN) {
  EXPECT_DOUBLE_EQ(
      PortfolioReturn(std::vector{0.5, 0.5}, std::vector{0.10, -0.02}), 0.04);
  const std::vector<Ticker> u{{"A", 0.1}, {"B", 0.5}, {"C", -0.2}, {"D", 0.3}};
  const auto top = TopPerformers(u, 2);
  ASSERT_EQ(top.size(), 2u);
  EXPECT_EQ(top[0].symbol, "B");
  EXPECT_EQ(top[1].symbol, "D");
  EXPECT_EQ(TopPerformers(u, 10).size(), 4u);
  EXPECT_DOUBLE_EQ(SumOfGains(std::vector{0.1, -0.2, 0.3}), 0.4);
}

}  // namespace
}  // namespace cpp4q::hanson::ch05
