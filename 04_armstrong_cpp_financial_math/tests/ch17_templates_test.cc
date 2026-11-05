#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <list>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "ch17_templates/templates.h"

namespace cpp4q::armstrong::ch17 {
namespace {

TEST(SimpsonTest, ExactForCubicsAndFourthOrder) {
  EXPECT_NEAR(IntegrateSimpson([](double x) { return x * x * x - x; }, 0.0,
                               2.0, 2),
              2.0, 1e-14);
  const auto f = [](double x) { return std::exp(x); };
  const double exact = std::exp(1.0) - 1;
  const double e1 = IntegrateSimpson(f, 0.0, 1.0, 10) - exact;
  const double e2 = IntegrateSimpson(f, 0.0, 1.0, 20) - exact;
  EXPECT_NEAR(e1 / e2, 16.0, 0.1);
  // An odd count is rounded up to even.
  EXPECT_DOUBLE_EQ(IntegrateSimpson(f, 0.0, 1.0, 9),
                   IntegrateSimpson(f, 0.0, 1.0, 10));
  EXPECT_THROW(IntegrateSimpson(f, 0.0, 1.0, 1), std::invalid_argument);
}

TEST(SimpsonTest, WorksForEachFloatingType) {
  const auto pi_f = IntegrateSimpson(
      [](float x) { return 4.0f / (1.0f + x * x); }, 0.0f, 1.0f, 100);
  EXPECT_NEAR(pi_f, std::numbers::pi_v<float>, 1e-5f);
  const auto pi_l = IntegrateSimpson(
      [](long double x) { return 4.0L / (1.0L + x * x); }, 0.0L, 1.0L, 1000);
  EXPECT_NEAR(static_cast<double>(pi_l), std::numbers::pi, 1e-14);
}

TEST(SumOfTest, AnyRange) {
  EXPECT_EQ(SumOf(std::vector<int>{1, 2, 3}), 6.0);
  EXPECT_EQ(SumOf<long long>(std::list<int>{1'000'000'000, 2'000'000'000}),
            3'000'000'000LL);
  EXPECT_EQ(SumOf(std::array<float, 2>{0.5f, 0.25f}), 0.75);
}

TEST(FixedMatrixTest, CompileTimeAlgebra) {
  using M23 = FixedMatrix<int, 2, 3>;
  constexpr M23 a(std::array<int, 6>{1, 2, 3, 4, 5, 6});
  constexpr auto at = a.Transpose();
  static_assert(at.rows() == 3 && at.cols() == 2);
  static_assert(at(2, 1) == 6);
  constexpr auto p = a * at;
  static_assert(p == FixedMatrix<int, 2, 2>(std::array<int, 4>{14, 32, 32,
                                                               77}));
  static_assert(FixedMatrix<int, 3, 3>::Identity() * at == at);
  EXPECT_EQ(p(1, 0), 32);
}

TEST(InterpolatorTest, LinearAndFlat) {
  const LinearInterpolator curve(std::vector<double>{1, 2, 4},
                                 std::vector<double>{0.01, 0.02, 0.03});
  EXPECT_DOUBLE_EQ(curve(0.5), 0.01);
  EXPECT_DOUBLE_EQ(curve(1.5), 0.015);
  EXPECT_DOUBLE_EQ(curve(2), 0.02);
  EXPECT_DOUBLE_EQ(curve(3), 0.025);
  EXPECT_DOUBLE_EQ(curve(10), 0.03);
  const LinearInterpolator<float> f({0.f, 1.f}, {0.f, 2.f});
  EXPECT_FLOAT_EQ(f(0.25f), 0.5f);
  EXPECT_THROW(LinearInterpolator<double>({1, 1}, {0, 0}),
               std::invalid_argument);
  EXPECT_THROW(LinearInterpolator<double>({}, {}), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch17
