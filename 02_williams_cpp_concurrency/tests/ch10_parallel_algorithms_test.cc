#include <gtest/gtest.h>

#include <cmath>
#include <execution>
#include <limits>
#include <vector>

#include "ch10_parallel_algorithms/portfolio_algos.h"

namespace cpp4q::williams::ch10 {
namespace {

TEST(BondPvTest, ParBondPricesAtPar) {
  const Bond b{.id = 1, .notional = 100.0, .coupon = 0.05, .years = 10};
  EXPECT_NEAR(BondPv(b, 0.05), 100.0, 1e-10);
  EXPECT_GT(BondPv(b, 0.04), 100.0);
  EXPECT_LT(BondPv(b, 0.06), 100.0);
}

TEST(PortfolioAlgosTest, PoliciesAgree) {
  const auto book = MakeBondBook(50'000, 3);
  const double seq = PortfolioPv(std::execution::seq, book, 0.03);
  // Parallel reduction may reassociate, so allow rounding differences.
  EXPECT_NEAR(PortfolioPv(std::execution::par, book, 0.03), seq,
              1e-12 * std::abs(seq));
  EXPECT_NEAR(PortfolioPv(std::execution::par_unseq, book, 0.03), seq,
              1e-12 * std::abs(seq));

  std::vector<double> a(book.size()), b(book.size());
  PriceAll(std::execution::seq, book, a, 0.03);
  PriceAll(std::execution::par_unseq, book, b, 0.03);
  EXPECT_EQ(a, b);  // element-wise: no reassociation
}

TEST(PortfolioAlgosTest, SortIsDeterministic) {
  auto x = MakeBondBook(20'000, 9);
  auto y = x;
  SortByNotional(std::execution::seq, x);
  SortByNotional(std::execution::par, y);
  ASSERT_EQ(x.size(), y.size());
  for (std::size_t i = 0; i < x.size(); ++i) ASSERT_EQ(x[i].id, y[i].id);
  for (std::size_t i = 1; i < x.size(); ++i) {
    ASSERT_GE(x[i - 1].notional, x[i].notional);
  }
}

TEST(PortfolioAlgosTest, BreachesAndBadExposures) {
  std::vector<double> e{1.0, -5.0, 3.0, 10.0, -0.5};
  EXPECT_EQ(CountLimitBreaches(std::execution::par, e, 2.0), 3u);
  EXPECT_FALSE(HasBadExposure(std::execution::par, e));
  e.push_back(std::numeric_limits<double>::quiet_NaN());
  EXPECT_TRUE(HasBadExposure(std::execution::par, e));
}

TEST(PortfolioAlgosTest, BookIsReproducible) {
  const auto a = MakeBondBook(100, 5), b = MakeBondBook(100, 5);
  for (std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i].notional, b[i].notional);
    EXPECT_EQ(a[i].years, b[i].years);
  }
}

}  // namespace
}  // namespace cpp4q::williams::ch10
