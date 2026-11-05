#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ch16_matrix/matrix.h"
#include "ch16_matrix/multi_stock.h"

namespace cpp4q::armstrong::ch16 {
namespace {

Matrix RandomMatrix(std::size_t r, std::size_t c, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(-1, 1);
  Matrix m(r, c);
  for (std::size_t j = 0; j < c; ++j) {
    for (std::size_t i = 0; i < r; ++i) m(i, j) = u(rng);
  }
  return m;
}

TEST(MatrixTest, ConstructionAndAccess) {
  const Matrix m{{1, 2, 3}, {4, 5, 6}};
  EXPECT_EQ(m.rows(), 2u);
  EXPECT_EQ(m.cols(), 3u);
  EXPECT_EQ(m(1, 0), 4);
  EXPECT_EQ(m.data()[1], 4);  // column-major
  EXPECT_EQ(m.column(2)[1], 6);
  EXPECT_THROW(m.at(2, 0), std::out_of_range);
  EXPECT_THROW((Matrix{{1, 2}, {3}}), std::invalid_argument);
  EXPECT_EQ(Matrix(2, 2, 7.0)(1, 1), 7);
  EXPECT_EQ(Matrix().size(), 0u);
}

TEST(MatrixTest, RuleOfFive) {
  Matrix a{{1, 2}, {3, 4}};
  Matrix b = a;  // deep copy
  b(0, 0) = 9;
  EXPECT_EQ(a(0, 0), 1);
  Matrix c;
  c = a;
  EXPECT_EQ(c, a);
  c = c;  // self-assignment  // NOLINT
  EXPECT_EQ(c, a);
  const double* buffer = a.data();
  Matrix d = std::move(a);
  EXPECT_EQ(d.data(), buffer);  // stolen, not copied
  EXPECT_EQ(a.size(), 0u);      // NOLINT(bugprone-use-after-move)
  Matrix e;
  e = std::move(d);
  EXPECT_EQ(e.data(), buffer);
  static_assert(std::is_nothrow_move_constructible_v<Matrix>);
  static_assert(std::is_nothrow_move_assignable_v<Matrix>);
}

TEST(MatrixTest, Arithmetic) {
  const Matrix a{{1, 2}, {3, 4}};
  const Matrix b{{5, 6}, {7, 8}};
  EXPECT_EQ(a + b, (Matrix{{6, 8}, {10, 12}}));
  EXPECT_EQ(b - a, (Matrix{{4, 4}, {4, 4}}));
  EXPECT_EQ(2.0 * a, (Matrix{{2, 4}, {6, 8}}));
  EXPECT_EQ(a * b, (Matrix{{19, 22}, {43, 50}}));
  EXPECT_EQ(a.Transpose(), (Matrix{{1, 3}, {2, 4}}));
  const Matrix r{{1, 2, 3}};
  EXPECT_EQ((r * r.Transpose()), (Matrix{{14}}));
  const std::vector<double> x{1, 1};
  EXPECT_EQ(a * std::span<const double>(x), (std::vector<double>{3, 7}));
  EXPECT_THROW(a * r, std::invalid_argument);
  EXPECT_THROW(a + r, std::invalid_argument);
  std::ostringstream out;
  out << a;
  EXPECT_EQ(out.str(), "[1, 2;\n 3, 4]\n");
}

TEST(MatrixTest, ProductIsAssociativeAndIdentityNeutral) {
  const Matrix a = RandomMatrix(7, 5, 1);
  const Matrix b = RandomMatrix(5, 9, 2);
  const Matrix c = RandomMatrix(9, 4, 3);
  EXPECT_LT(MaxAbsDifference((a * b) * c, a * (b * c)), 1e-13);
  EXPECT_EQ(Matrix::Identity(7) * a, a);
  EXPECT_LT(MaxAbsDifference((a * b).Transpose(), b.Transpose() * a.Transpose()),
            1e-15);
}

TEST(MatrixTest, Cholesky) {
  const Matrix x = RandomMatrix(6, 6, 4);
  const Matrix spd = x * x.Transpose() + Matrix::Identity(6);
  const Matrix l = Cholesky(spd);
  EXPECT_LT(MaxAbsDifference(l * l.Transpose(), spd), 1e-13);
  for (std::size_t i = 0; i < 6; ++i) {
    for (std::size_t j = i + 1; j < 6; ++j) EXPECT_EQ(l(i, j), 0);
  }
  EXPECT_THROW(Cholesky(Matrix{{1, 2}, {2, 1}}), std::domain_error);
  EXPECT_THROW(Cholesky(Matrix{{1, 0.5}, {0.4, 1}}), std::domain_error);
}

TEST(MatrixTest, SolveWithPivoting) {
  // Needs a row swap: a(0,0) = 0.
  const Matrix a{{0, 2, 1}, {1, 1, 1}, {2, 1, 3}};
  const std::vector<double> expected{1, -2, 3};
  const std::vector<double> b = a * std::span<const double>(expected);
  const auto x = Solve(a, b);
  for (std::size_t i = 0; i < 3; ++i) EXPECT_NEAR(x[i], expected[i], 1e-14);
  const Matrix big = RandomMatrix(40, 40, 5);
  const std::vector<double> ones(40, 1.0);
  const auto y = Solve(big, big * std::span<const double>(ones));
  for (double v : y) EXPECT_NEAR(v, 1.0, 1e-10);
  EXPECT_THROW(Solve(Matrix{{1, 2}, {2, 4}}, std::vector<double>{1, 2}),
               std::domain_error);
}

TEST(MultiStockTest, MargrabeAndCorrelation) {
  const Matrix rho{{1, 0.6}, {0.6, 1}};
  const MultiStockModel model({100, 90}, {0.25, 0.15}, rho, 0.04);
  Rng rng(16);
  const Estimate e = PriceExchangeOption(model, 1.5, 200'000, rng);
  const double exact = MargrabePrice(100, 90, 0.25, 0.15, 0.6, 1.5);
  EXPECT_TRUE(e.Contains(exact)) << e.price << " vs " << exact;
  // Sample correlation of log returns.
  std::vector<double> s(2);
  double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
  constexpr int kN = 100'000;
  for (int i = 0; i < kN; ++i) {
    model.SimulateTerminal(1, rng, s);
    const double x = std::log(s[0]), y = std::log(s[1]);
    sx += x, sy += y, sxx += x * x, syy += y * y, sxy += x * y;
  }
  const double cov = sxy / kN - sx / kN * sy / kN;
  const double corr = cov / std::sqrt((sxx / kN - sx * sx / kN / kN) *
                                      (syy / kN - sy * sy / kN / kN));
  EXPECT_NEAR(corr, 0.6, 4 * (1 - 0.36) / std::sqrt(kN));
}

TEST(MultiStockTest, OneStockBasketIsBlackScholes) {
  const MultiStockModel model({100}, {0.2}, Matrix{{1}}, 0.05);
  Rng rng(17);
  const std::vector<double> w{1};
  const Estimate e = PriceBasketCall(model, w, 100, 1, 200'000, rng);
  EXPECT_TRUE(e.Contains(10.450583572185565)) << e.price;
  EXPECT_THROW(MultiStockModel({100, 100}, {0.2}, Matrix{{1}}, 0.05),
               std::invalid_argument);
  EXPECT_THROW(MultiStockModel({100}, {0.2}, Matrix{{2}}, 0.05),
               std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch16
