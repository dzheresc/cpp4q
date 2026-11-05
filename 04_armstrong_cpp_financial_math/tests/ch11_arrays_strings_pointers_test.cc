#include <gtest/gtest.h>

#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "ch11_arrays_strings_pointers/arrays.h"

namespace cpp4q::armstrong::ch11 {
namespace {

TEST(PointersTest, SumAndReverse) {
  double a[] = {1, 2, 3, 4, 5};
  EXPECT_EQ(SumArray(a, 5), 15);
  EXPECT_EQ(SumArray(a, 0), 0);
  ReverseInPlace(a, a + 5);
  EXPECT_EQ(std::vector<double>(a, a + 5), (std::vector<double>{5, 4, 3, 2, 1}));
  ReverseInPlace(a, a + 4);
  EXPECT_EQ(std::vector<double>(a, a + 5), (std::vector<double>{2, 3, 4, 5, 1}));
  ReverseInPlace(a, a);  // empty range
}

TEST(CStringTest, LengthAndCopy) {
  EXPECT_EQ(CStringLength(""), 0u);
  EXPECT_EQ(CStringLength("delta"), 5u);
  char buf[6];
  EXPECT_EQ(CopyCString(buf, sizeof buf, "gamma"), 5u);
  EXPECT_STREQ(buf, "gamma");
  EXPECT_EQ(CopyCString(buf, sizeof buf, "volatility"), 10u);
  EXPECT_STREQ(buf, "volat");  // truncated, still terminated
  EXPECT_EQ(CopyCString(buf, 0, "x"), 1u);
}

TEST(BufferTest, OwnsAndMoves) {
  DoubleBuffer b(4);
  for (std::size_t i = 0; i < b.size(); ++i) b[i] = static_cast<double>(i);
  EXPECT_EQ(SumArray(b.data(), b.size()), 6);
  DoubleBuffer moved = std::move(b);
  EXPECT_EQ(moved.span().size(), 4u);
  EXPECT_EQ(std::accumulate(moved.span().begin(), moved.span().end(), 0.0), 6);
}

TEST(StringTest, SplitAndTrim) {
  const auto f = Split("a,,b,", ',');
  ASSERT_EQ(f.size(), 4u);
  EXPECT_EQ(f[0], "a");
  EXPECT_EQ(f[1], "");
  EXPECT_EQ(f[2], "b");
  EXPECT_EQ(f[3], "");
  EXPECT_EQ(Split("", ',').size(), 1u);
  EXPECT_EQ(Trim("  x y \t\r"), "x y");
  EXPECT_EQ(Trim("   "), "");
}

TEST(CsvTest, ReadsPricesAndReportsBadLines) {
  std::istringstream good("date,close\n\n2024-01-02, 100.5\r\n2024-01-03,101\n");
  const auto r = ReadPriceCsv(good);
  ASSERT_EQ(r.size(), 2u);
  EXPECT_EQ(r[0].date, "2024-01-02");
  EXPECT_EQ(r[0].close, 100.5);
  std::istringstream bad("date,close\n2024-01-02,abc\n");
  try {
    ReadPriceCsv(bad);
    FAIL();
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find("line 2"), std::string::npos);
  }
  std::istringstream fields("date,close\n2024-01-02,1,2\n");
  EXPECT_THROW(ReadPriceCsv(fields), std::runtime_error);
}

TEST(ReturnsTest, VolatilityOfAlternatingReturns) {
  // log returns alternate +x, -x: sample std = x * sqrt(n / (n - 1)).
  const double x = 0.01;
  std::vector<double> p{100};
  for (int i = 0; i < 10; ++i) p.push_back(p.back() * std::exp(i % 2 ? -x : x));
  const auto r = LogReturns(p);
  ASSERT_EQ(r.size(), 10u);
  EXPECT_NEAR(r[0], x, 1e-15);
  EXPECT_NEAR(HistoricalVolatility(p, 1), x * std::sqrt(10.0 / 9), 1e-14);
  EXPECT_THROW(LogReturns(std::vector<double>{1}), std::invalid_argument);
  EXPECT_THROW(LogReturns(std::vector<double>{1, -1}), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch11
