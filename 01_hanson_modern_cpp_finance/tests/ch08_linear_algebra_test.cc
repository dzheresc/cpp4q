#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <stdexcept>
#include <valarray>
#include <vector>

#include "ch08_linear_algebra/decompositions.h"
#include "ch08_linear_algebra/matrix.h"
#include "ch08_linear_algebra/valarray_ops.h"

namespace cpp4q::hanson::ch08 {
namespace {

Matrix Random(std::size_t r, std::size_t c, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  Matrix m(r, c);
  for (std::size_t i = 0; i < r; ++i) {
    for (std::size_t j = 0; j < c; ++j) m(i, j) = u(rng);
  }
  return m;
}

TEST(MatrixTest, BasicsAndShapes) {
  const Matrix a{{1, 2, 3}, {4, 5, 6}};
  EXPECT_EQ(a.rows(), 2u);
  EXPECT_EQ(a(1, 2), 6.0);
  EXPECT_EQ(a.Transpose(), (Matrix{{1, 4}, {2, 5}, {3, 6}}));
  EXPECT_EQ(a + a, a * 2.0);
  EXPECT_EQ(a - a, Matrix(2, 3));
  EXPECT_THROW((Matrix{{1, 2}, {3}}), std::invalid_argument);
  EXPECT_THROW((void)(a * a), std::invalid_argument);
  EXPECT_THROW((void)(a + a.Transpose()), std::invalid_argument);
}

TEST(MatrixTest, ProductsAgree) {
  const Matrix a = Random(37, 23, 1);
  const Matrix b = Random(23, 41, 2);
  const Matrix c = MultiplyNaive(a, b);
  EXPECT_LT(MaxAbsDiff(Multiply(a, b), c), 1e-12);
  EXPECT_EQ(Multiply(Matrix::Identity(23), b), b);
  const std::vector<double> x(23, 1.0);
  const auto y = Multiply(a, x);
  double row0 = 0.0;
  for (std::size_t k = 0; k < 23; ++k) row0 += a(0, k);
  EXPECT_NEAR(y[0], row0, 1e-12);
}

TEST(CholeskyTest, Reconstructs) {
  const Matrix r = Random(20, 20, 3);
  const Matrix spd = r * r.Transpose() + Matrix::Identity(20);
  const Matrix l = Cholesky(spd);
  EXPECT_LT(MaxAbsDiff(l * l.Transpose(), spd), 1e-12);
  for (std::size_t i = 0; i < 20; ++i) {
    for (std::size_t j = i + 1; j < 20; ++j) EXPECT_EQ(l(i, j), 0.0);
  }
  EXPECT_THROW((void)Cholesky(Matrix{{1, 2}, {2, 1}}), std::domain_error);
  EXPECT_THROW((void)Cholesky(Matrix{{1, 2}, {0, 1}}), std::domain_error);
}

TEST(LuTest, SolveInverseDeterminant) {
  const Matrix a = Random(15, 15, 4) + Matrix::Identity(15) * 5.0;
  const LuDecomposition lu(a);
  const auto x_true = std::vector<double>(15, 2.0);
  const auto b = Multiply(a, x_true);
  const auto x = lu.Solve(b);
  for (double v : x) EXPECT_NEAR(v, 2.0, 1e-12);
  EXPECT_LT(MaxAbsDiff(a * lu.Inverse(), Matrix::Identity(15)), 1e-12);
  EXPECT_NEAR(LuDecomposition(Matrix{{0, 2}, {3, 4}}).Determinant(), -6.0,
              1e-15);
  EXPECT_THROW(LuDecomposition(Matrix{{1, 2}, {2, 4}}), std::domain_error);
}

TEST(CorrelateTest, EmpiricalCorrelation) {
  const Matrix corr{{1.0, 0.8}, {0.8, 1.0}};
  const Matrix l = Cholesky(corr);
  std::mt19937_64 rng(5);
  std::normal_distribution<double> n;
  double sxy = 0.0;
  constexpr int kN = 200'000;
  for (int i = 0; i < kN; ++i) {
    const auto y = Correlate(l, std::vector{n(rng), n(rng)});
    sxy += y[0] * y[1];
  }
  EXPECT_NEAR(sxy / kN, 0.8, 0.01);
}

TEST(ValarrayTest, MatchesMatrix) {
  const Matrix a = Random(6, 4, 6);
  const Matrix b = Random(4, 5, 7);
  ValMatrix va{6, 4, std::valarray<double>(a.data().data(), 24)};
  ValMatrix vb{4, 5, std::valarray<double>(b.data().data(), 20)};
  const ValMatrix vc = MultiplyValarray(va, vb);
  const Matrix c = a * b;
  for (std::size_t i = 0; i < 30; ++i)
    EXPECT_NEAR(vc.data[i], c.data()[i], 1e-14);
  EXPECT_EQ(Col(va, 1)[2], a(2, 1));
  const auto s = GbmStep({100.0, 100.0}, {0.0, 1.0}, 0.0, 0.1);
  EXPECT_DOUBLE_EQ(s[0], 100.0);
  EXPECT_DOUBLE_EQ(s[1], 100.0 * std::exp(0.1));
}

}  // namespace
}  // namespace cpp4q::hanson::ch08
