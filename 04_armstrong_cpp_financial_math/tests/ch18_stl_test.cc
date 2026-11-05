#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "ch18_stl/stl.h"

namespace cpp4q::armstrong::ch18 {
namespace {

TEST(VarTest, SmallSampleByHand) {
  std::vector<double> losses(100);
  std::iota(losses.begin(), losses.end(), 1.0);  // 1..100
  std::shuffle(losses.begin(), losses.end(), std::mt19937(1));
  EXPECT_EQ(HistoricalVaR(losses, 0.95), 95);
  EXPECT_EQ(HistoricalVaR(losses, 0.99), 99);
  EXPECT_EQ(HistoricalVaR(losses, 0.951), 96);
  EXPECT_DOUBLE_EQ(HistoricalExpectedShortfall(losses, 0.95),
                   (95 + 96 + 97 + 98 + 99 + 100) / 6.0);
  EXPECT_EQ(HistoricalVaR(std::vector<double>{3}, 0.5), 3);
  EXPECT_THROW(HistoricalVaR(std::vector<double>{}, 0.9),
               std::invalid_argument);
  EXPECT_THROW(HistoricalVaR(losses, 1.0), std::invalid_argument);
}

TEST(VarTest, NormalQuantiles) {
  std::mt19937_64 rng(18);
  std::normal_distribution<double> z;
  std::vector<double> losses(400'000);
  for (double& x : losses) x = z(rng);
  EXPECT_NEAR(HistoricalVaR(losses, 0.99), 2.326348, 0.03);
  // ES of a standard normal at 99%: phi(2.326) / 0.01 = 2.665.
  EXPECT_NEAR(HistoricalExpectedShortfall(losses, 0.99), 2.665214, 0.03);
}

TEST(HistogramTest, Bins) {
  const std::vector<double> v{-0.5, 0.1, 0.9, 1.0, 2.5, -2.0};
  const auto h = Histogram(v, 1.0);
  EXPECT_EQ(h.at(-1), 1u);
  EXPECT_EQ(h.at(0), 2u);
  EXPECT_EQ(h.at(1), 1u);
  EXPECT_EQ(h.at(2), 1u);
  EXPECT_EQ(h.at(-2), 1u);
  EXPECT_EQ(h.size(), 5u);
  EXPECT_THROW(Histogram(v, 0), std::invalid_argument);
}

TEST(TradesTest, AggregateAndRank) {
  const std::vector<Trade> trades{{"B", 10, 5}, {"A", 5, 2},  {"B", -4, 6},
                                  {"C", 3, 1},  {"C", -3, 2}, {"D", -100, 1}};
  const auto book = AggregateTrades(trades);
  ASSERT_EQ(book.size(), 3u);  // C nets to zero
  EXPECT_EQ(book.begin()->first, "A");
  EXPECT_EQ(book.at("B").quantity, 6);
  EXPECT_EQ(book.at("B").cost, 26);
  const std::map<std::string, double> marks{{"A", 2}, {"B", 6}, {"D", 1.5}};
  const auto top = LargestExposures(book, marks, 2);
  ASSERT_EQ(top.size(), 2u);
  EXPECT_EQ(top[0].first, "D");
  EXPECT_EQ(top[0].second, -150);
  EXPECT_EQ(top[1].first, "B");
  EXPECT_EQ(LargestExposures(book, marks, 10).size(), 3u);
  EXPECT_THROW(LargestExposures(book, {{"A", 1}}, 1), std::invalid_argument);
}

TEST(RollingTest, MeanAndDates) {
  const std::vector<double> v{1, 2, 3, 4, 5};
  EXPECT_EQ(RollingMean(v, 2), (std::vector<double>{1.5, 2.5, 3.5, 4.5}));
  EXPECT_EQ(RollingMean(v, 5), (std::vector<double>{3}));
  EXPECT_TRUE(RollingMean(v, 6).empty());
  EXPECT_THROW(RollingMean(v, 0), std::invalid_argument);
  const std::vector<std::string> dates{"2024-03-01", "2024-01-15",
                                       "2024-03-01", "2023-12-31"};
  const auto unique = UniqueDates(dates);
  EXPECT_EQ(unique.size(), 3u);
  EXPECT_EQ(*unique.begin(), "2023-12-31");
}

}  // namespace
}  // namespace cpp4q::armstrong::ch18
