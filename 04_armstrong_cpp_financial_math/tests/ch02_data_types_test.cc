#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "ch02_data_types/numerics.h"

namespace cpp4q::armstrong::ch02 {
namespace {

constexpr auto kMax = std::numeric_limits<std::int64_t>::max();
constexpr auto kMin = std::numeric_limits<std::int64_t>::min();

TEST(CheckedTest, Overflow) {
  EXPECT_EQ(CheckedAdd(2, 3), 5);
  EXPECT_FALSE(CheckedAdd(kMax, 1));
  EXPECT_FALSE(CheckedAdd(kMin, -1));
  EXPECT_EQ(CheckedAdd(kMax, kMin), -1);
  EXPECT_FALSE(CheckedMultiply(kMax / 2 + 1, 2));
  EXPECT_FALSE(CheckedMultiply(kMin, -1));
  EXPECT_EQ(CheckedMultiply(-4, 5), -20);
}

TEST(FloorDivTest, SignsAndZero) {
  EXPECT_EQ(FloorDiv(7, 2), 3);
  EXPECT_EQ(FloorDiv(-7, 2), -4);
  EXPECT_EQ(FloorDiv(7, -2), -4);
  EXPECT_EQ(FloorDiv(-8, 2), -4);
  EXPECT_EQ(FloorMod(-7, 2), 1);
  EXPECT_EQ(FloorMod(7, -2), -1);
  EXPECT_EQ(FloorMod(-1, 7), 6);  // yesterday from Sunday (0) is Saturday
  EXPECT_THROW(FloorDiv(1, 0), std::domain_error);
}

TEST(EpsilonTest, MatchesNumericLimits) {
  EXPECT_EQ(MachineEpsilon<float>(), std::numeric_limits<float>::epsilon());
  EXPECT_EQ(MachineEpsilon<double>(), std::numeric_limits<double>::epsilon());
  // MachineEpsilon measures the arithmetic actually executed. Valgrind
  // runs x87 long double at double precision, so there it finds the
  // double epsilon; check against what this machine really does.
  volatile long double one_plus = 1.0L;
  one_plus += std::numeric_limits<long double>::epsilon();
  const bool full_precision = one_plus != 1.0L;
  EXPECT_EQ(MachineEpsilon<long double>(),
            full_precision ? std::numeric_limits<long double>::epsilon()
                           : static_cast<long double>(
                                 std::numeric_limits<double>::epsilon()));
}

TEST(SumTest, CompensatedBeatsNaive) {
  const std::vector<double> tenths(1'000'000, 0.1);
  const double exact = 100000.0;
  EXPECT_GT(std::abs(NaiveSum(tenths) - exact), 1e-7);
  EXPECT_LT(std::abs(CompensatedSum(tenths) - exact), 1e-9);
  // Neumaier handles a term larger than the running sum; Kahan does not.
  const std::vector<double> tricky{1.0, 1e100, 1.0, -1e100};
  EXPECT_EQ(NaiveSum(tricky), 0.0);
  EXPECT_EQ(CompensatedSum(tricky), 2.0);
}

TEST(CancellationTest, Expm1) {
  const double x = 1e-12;
  EXPECT_GT(std::abs(NaiveExpMinusOne(x) - x) / x, 1e-5);
  EXPECT_LT(std::abs(std::expm1(x) - x) / x, 1e-11);
}

TEST(ApproxEqualTest, Tolerances) {
  EXPECT_TRUE(ApproxEqual(1.0, 1.0 + 1e-13));
  EXPECT_FALSE(ApproxEqual(1.0, 1.0 + 1e-9));
  EXPECT_FALSE(ApproxEqual(0.0, 1e-300));
  EXPECT_TRUE(ApproxEqual(0.0, 1e-300, 1e-12, 1e-200));
  EXPECT_FALSE(ApproxEqual(std::nan(""), std::nan("")));
}

}  // namespace
}  // namespace cpp4q::armstrong::ch02
