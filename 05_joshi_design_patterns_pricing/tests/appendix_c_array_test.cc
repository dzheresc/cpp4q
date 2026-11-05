#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "appendix_c_array/mj_array.h"

namespace cpp4q::joshi {
namespace {

static_assert(std::is_nothrow_move_constructible_v<MjArray>);
static_assert(std::is_nothrow_move_assignable_v<MjArray>);

TEST(MjArrayTest, ValueSemantics) {
  MjArray a{1, 2, 3};
  MjArray b = a;
  b[0] = 10;
  EXPECT_EQ(a[0], 1);
  a = b;
  EXPECT_EQ(a, b);
  const MjArray& self = a;
  a = self;  // self-assignment
  EXPECT_EQ(a, b);
  const MjArray moved = std::move(b);
  EXPECT_EQ(moved, a);
}

TEST(MjArrayTest, Arithmetic) {
  const MjArray a{1, 2, 3, 4};
  const MjArray b{4, 3, 2, 1};
  EXPECT_EQ(a + b, (MjArray{5, 5, 5, 5}));
  EXPECT_EQ(a - b, (MjArray{-3, -1, 1, 3}));
  EXPECT_EQ(a * b, (MjArray{4, 6, 6, 4}));
  EXPECT_EQ(2.0 * a, (MjArray{2, 4, 6, 8}));
  MjArray c = a;
  c /= 2.0;
  c -= 0.5;
  EXPECT_EQ(c, (MjArray{0, 0.5, 1, 1.5}));
  EXPECT_EQ(a.Sum(), 10);
  EXPECT_EQ(a.Min(), 1);
  EXPECT_EQ(b.Max(), 4);
  EXPECT_THROW(c += MjArray(3), std::invalid_argument);
  EXPECT_THROW(MjArray().Min(), std::logic_error);
}

TEST(MjArrayTest, ResizeKeepsCapacity) {
  MjArray a(8, 1.0);
  const double* buffer = a.data();
  a.resize(3);
  a.resize(8);
  EXPECT_EQ(a.data(), buffer);
  EXPECT_EQ(a.capacity(), 8u);
  a.resize(9);
  EXPECT_EQ(a.size(), 9u);
  EXPECT_EQ(a.capacity(), 9u);
  // Copy assignment into a big enough array reuses its buffer.
  MjArray big(20);
  const double* big_buffer = big.data();
  const MjArray small{1, 2};
  big = small;
  EXPECT_EQ(big.data(), big_buffer);
  EXPECT_EQ(big.size(), 2u);
}

TEST(MjArrayTest, AddScaledMatchesOperators) {
  MjArray y{1, 2, 3};
  const MjArray x{4, 5, 6};
  const MjArray expected = y + 0.5 * x;
  y.AddScaled(0.5, x);
  EXPECT_EQ(y, expected);
  EXPECT_THROW(y.AddScaled(1.0, MjArray(2)), std::invalid_argument);
}

TEST(MjArrayTest, ApplyAndSpan) {
  MjArray a{1, 4, 9};
  a.Apply([](double x) { return std::sqrt(x); });
  EXPECT_EQ(a, (MjArray{1, 2, 3}));
  double sum = 0;
  for (const double x : a.span()) sum += x;
  EXPECT_EQ(sum, 6);
}

}  // namespace
}  // namespace cpp4q::joshi
