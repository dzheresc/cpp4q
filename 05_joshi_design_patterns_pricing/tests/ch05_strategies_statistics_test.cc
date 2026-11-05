#include <initializer_list>
#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch05_strategies_statistics/statistics.h"

namespace cpp4q::joshi::ch05 {
namespace {

TEST(WrapperTest, DeepCopyAndEmptyState) {
  Wrapper<StatisticsMc> a(StatisticsMean{});
  a->DumpOneResult(2.0);
  Wrapper<StatisticsMc> b = a;
  b->DumpOneResult(4.0);
  EXPECT_EQ(a->GetResultsSoFar()[0][0], 2.0);
  EXPECT_EQ(b->GetResultsSoFar()[0][0], 3.0);
  Wrapper<StatisticsMc> empty;
  EXPECT_FALSE(empty);
  Wrapper<StatisticsMc> copy_of_empty = empty;
  EXPECT_FALSE(copy_of_empty);
  b = empty;
  EXPECT_FALSE(b);
}

TEST(StatisticsTest, MomentsOfKnownData) {
  StatisticsMoments m;
  for (const double x : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
    m.DumpOneResult(x + 1e9);  // a large offset must not cost precision
  }
  const auto row = m.GetResultsSoFar()[0];
  EXPECT_NEAR(row[0] - 1e9, 5.0, 1e-6);
  EXPECT_NEAR(row[2], 32.0 / 7.0, 1e-6);  // sample variance
  EXPECT_NEAR(row[1], std::sqrt(32.0 / 7.0 / 8.0), 1e-6);
  // Population skewness and excess kurtosis of this data set.
  EXPECT_NEAR(row[3], 0.65625, 1e-6);
  EXPECT_NEAR(row[4], 2.78125 - 3.0, 1e-6);
}

TEST(StatisticsTest, MomentsSurviveAnOutlierFirstResult) {
  // The shift is the first result; make it 50 standard deviations out and
  // compare with a two-pass computation.
  std::vector<double> xs{1000.0};
  for (int i = 0; i < 9999; ++i) xs.push_back(std::sin(i * 0.37) * 20.0);
  StatisticsMoments m;
  double mean = 0;
  for (const double x : xs) {
    m.DumpOneResult(x);
    mean += x;
  }
  mean /= static_cast<double>(xs.size());
  double c2 = 0, c3 = 0, c4 = 0;
  for (const double x : xs) {
    const double d = x - mean;
    c2 += d * d;
    c3 += d * d * d;
    c4 += d * d * d * d;
  }
  const double n = static_cast<double>(xs.size());
  const auto row = m.GetResultsSoFar()[0];
  EXPECT_NEAR(row[0], mean, 1e-12 * std::fabs(mean) + 1e-12);
  EXPECT_NEAR(row[2], c2 / (n - 1), 1e-9 * c2 / n);
  EXPECT_NEAR(row[3], std::sqrt(n) * c3 / std::pow(c2, 1.5), 1e-8);
  EXPECT_NEAR(row[4], n * c4 / (c2 * c2) - 3, 1e-7);
}

TEST(StatisticsTest, Quantiles) {
  StatisticsQuantiles q({0.01, 0.5, 1.0});
  for (int i = 1; i <= 1000; ++i) q.DumpOneResult(1001 - i);
  const auto row = q.GetResultsSoFar()[0];
  EXPECT_EQ(row[0], 10);
  EXPECT_EQ(row[1], 500);
  EXPECT_EQ(row[2], 1000);
  EXPECT_THROW(StatisticsQuantiles({0.0}), std::invalid_argument);
}

TEST(ConvergenceTableTest, SnapshotsAtPowersOfTwo) {
  ConvergenceTable table{Wrapper<StatisticsMc>(StatisticsMean{})};
  for (int i = 1; i <= 10; ++i) table.DumpOneResult(i);
  const ResultTable t = table.GetResultsSoFar();
  ASSERT_EQ(t.size(), 5u);  // 1, 2, 4, 8, and the final 10
  const std::vector<double> counts{1, 2, 4, 8, 10};
  const std::vector<double> means{1, 1.5, 2.5, 4.5, 5.5};
  for (std::size_t i = 0; i < t.size(); ++i) {
    EXPECT_EQ(t[i][1], counts[i]);
    EXPECT_EQ(t[i][0], means[i]);
  }
  // At a power of two the last row is not duplicated.
  ConvergenceTable t8{Wrapper<StatisticsMc>(StatisticsMean{})};
  for (int i = 1; i <= 8; ++i) t8.DumpOneResult(i);
  EXPECT_EQ(t8.GetResultsSoFar().size(), 4u);
}

TEST(StatisticsMcTest, GathererSeesDiscountedPayOffs) {
  const ch04::VanillaOption put(ch04::PayOffPut(100), 1.0);
  StatisticsMoments stats;
  Rng rng(5);
  SimpleMonteCarlo5(put, 100, 0.2, 0.05, 100'000, stats, rng);
  const auto row = stats.GetResultsSoFar()[0];
  EXPECT_NEAR(row[0], BlackScholesPut(100, 100, 0.05, 0, 0.2, 1), 4 * row[1]);
  EXPECT_GT(row[3], 0.5);  // put pay-offs are right-skewed
}

}  // namespace
}  // namespace cpp4q::joshi::ch05
