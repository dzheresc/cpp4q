#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

#include "ch04_stl_containers/order_book.h"
#include "ch04_stl_containers/portfolio.h"
#include "ch04_stl_containers/rolling_window.h"

namespace cpp4q::hanson::ch04 {
namespace {

TEST(OrderBookTest, BestLevelsAndDepth) {
  OrderBook book;
  EXPECT_FALSE(book.Mid());
  book.Update(Side::kBid, 99.0, 10);
  book.Update(Side::kBid, 99.5, 5);
  book.Update(Side::kAsk, 100.5, 7);
  book.Update(Side::kAsk, 100.0, 3);
  EXPECT_EQ(book.BestBid(), (Level{99.5, 5}));
  EXPECT_EQ(book.BestAsk(), (Level{100.0, 3}));
  EXPECT_DOUBLE_EQ(*book.Mid(), 99.75);
  EXPECT_EQ(book.Depth(Side::kBid, 5),
            (std::vector<Level>{{99.5, 5}, {99.0, 10}}));
}

TEST(OrderBookTest, CancelRemovesEmptyLevel) {
  OrderBook book;
  book.Update(Side::kAsk, 101.0, 4);
  book.Update(Side::kAsk, 101.0, -4);
  EXPECT_EQ(book.level_count(Side::kAsk), 0u);
}

TEST(OrderBookTest, MarketOrderWalksTheBook) {
  OrderBook book;
  book.Update(Side::kAsk, 100.0, 3);
  book.Update(Side::kAsk, 101.0, 5);
  const auto fills = book.ExecuteMarket(Side::kBid, 6);
  ASSERT_EQ(fills.size(), 2u);
  EXPECT_EQ(fills[0].quantity, 3);
  EXPECT_EQ(fills[1].price, 101.0);
  EXPECT_EQ(book.BestAsk(), (Level{101.0, 2}));
  EXPECT_EQ(book.ExecuteMarket(Side::kBid, 100).size(), 1u);
  EXPECT_FALSE(book.BestAsk());
}

TEST(PortfolioTest, AverageCostAndRealisedPnl) {
  Portfolio pf;
  pf.Trade("XYZ", 100, 10.0);
  pf.Trade("XYZ", 100, 20.0);
  ASSERT_NE(pf.Find("XYZ"), nullptr);
  EXPECT_DOUBLE_EQ(pf.Find("XYZ")->average_cost, 15.0);
  pf.Trade("XYZ", -50, 25.0);
  EXPECT_DOUBLE_EQ(pf.realized_pnl(), 500.0);
  EXPECT_DOUBLE_EQ(pf.Find("XYZ")->quantity, 150.0);
  pf.Trade("XYZ", -150, 15.0);
  EXPECT_EQ(pf.Find("XYZ"), nullptr);
  EXPECT_DOUBLE_EQ(pf.realized_pnl(), 500.0);
}

TEST(PortfolioTest, PositionFlipResetsCost) {
  Portfolio pf;
  pf.Trade("ABC", 10, 100.0);
  pf.Trade("ABC", -30, 110.0);
  EXPECT_DOUBLE_EQ(pf.realized_pnl(), 100.0);
  EXPECT_DOUBLE_EQ(pf.Find("ABC")->quantity, -20.0);
  EXPECT_DOUBLE_EQ(pf.Find("ABC")->average_cost, 110.0);
  const PriceMap px{{"ABC", 105.0}};
  EXPECT_DOUBLE_EQ(pf.UnrealizedPnl(px), 100.0);
  EXPECT_DOUBLE_EQ(pf.MarketValue(px), -2100.0);
}

TEST(PortfolioTest, MissingPriceThrowsAndTickersSorted) {
  Portfolio pf;
  pf.Trade("ZZZ", 1, 1.0);
  pf.Trade("AAA", 1, 1.0);
  pf.Trade("MMM", 0, 1.0);  // no-op
  EXPECT_EQ(pf.Tickers(), (std::vector<std::string>{"AAA", "ZZZ"}));
  EXPECT_THROW((void)pf.MarketValue(PriceMap{{"AAA", 1.0}}), std::out_of_range);
}

TEST(RollingWindowTest, IteratesOldestToNewest) {
  RollingWindow w(3);
  for (double x : {1.0, 2.0, 3.0, 4.0, 5.0}) w.Push(x);
  const std::vector<double> got(w.begin(), w.end());
  EXPECT_EQ(got, (std::vector<double>{3.0, 4.0, 5.0}));
  EXPECT_DOUBLE_EQ(w.Mean(), 4.0);
  EXPECT_DOUBLE_EQ(w.Variance(), 1.0);
  EXPECT_THROW(RollingWindow(0), std::invalid_argument);
}

TEST(RollingWindowTest, MatchesTwoPassAndIsStable) {
  // Large level, tiny dispersion: naive sum-of-squares loses all digits.
  std::mt19937_64 rng(1);
  std::normal_distribution<double> noise(0.0, 1e-3);
  RollingWindow w(50);
  std::vector<double> all;
  for (int i = 0; i < 10'025; ++i) {  // end mid-turn, not on a resync
    const double x = 1e6 + noise(rng);
    all.push_back(x);
    w.Push(x);
  }
  const auto tail = std::vector<double>(all.end() - 50, all.end());
  const double mean = std::accumulate(tail.begin(), tail.end(), 0.0) / 50.0;
  double ss = 0.0;
  for (double x : tail) ss += (x - mean) * (x - mean);
  EXPECT_NEAR(w.Mean(), mean, 1e-12 * mean);
  EXPECT_NEAR(w.Variance(), ss / 49.0, 1e-6 * ss / 49.0);
}

}  // namespace
}  // namespace cpp4q::hanson::ch04
