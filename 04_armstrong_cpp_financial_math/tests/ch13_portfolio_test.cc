#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <stdexcept>

#include "ch03_functions/matlib.h"
#include "ch13_portfolio/portfolio.h"

namespace cpp4q::armstrong::ch13 {
namespace {

using ch10::EuropeanCall;
using ch10::EuropeanPut;

TEST(PortfolioTest, PricesAndQuantities) {
  const BlackScholesModel m;
  Portfolio p;
  const auto call = std::make_shared<EuropeanCall>(100, 1);
  EXPECT_EQ(p.Add(2, call), 0u);
  EXPECT_EQ(p.Add(-3, std::make_shared<Stock>()), 1u);
  EXPECT_NEAR(p.Price(m), 2 * call->Price(m) - 300, 1e-12);
  p.SetQuantity(1, 0);
  EXPECT_EQ(p.quantity(1), 0);
  EXPECT_NEAR(p.Price(m), 2 * call->Price(m), 1e-12);
  const auto values = p.PositionValues(m);
  ASSERT_EQ(values.size(), 2u);
  EXPECT_EQ(values[1], 0);
  EXPECT_THROW(p.SetQuantity(5, 1), std::out_of_range);
  EXPECT_THROW(p.Add(1, nullptr), std::invalid_argument);
  EXPECT_EQ(Portfolio().Price(m), 0);
}

TEST(PortfolioTest, SharedSecurityInTwoPortfolios) {
  const auto call = std::make_shared<EuropeanCall>(100, 1);
  Portfolio a, b;
  a.Add(1, call);
  b.Add(-1, call);
  EXPECT_EQ(call.use_count(), 3);
  const BlackScholesModel m;
  EXPECT_DOUBLE_EQ(a.Price(m) + b.Price(m), 0);
}

TEST(PortfolioTest, PutCallParityIsWorthNothingAndHasNoGreeks) {
  BlackScholesModel m;
  m.date = 0.3;
  for (double s : {60.0, 100.0, 160.0}) {
    m.stock_price = s;
    const Portfolio p = PutCallParityPortfolio(105, 1.3);
    EXPECT_NEAR(p.Price(m), 0, 1e-11) << s;
    EXPECT_NEAR(p.Delta(m), 0, 1e-7);
    EXPECT_NEAR(p.Vega(m), 0, 1e-7);
  }
}

TEST(PortfolioTest, GreeksMatchClosedForms) {
  const BlackScholesModel m;
  Portfolio straddle;
  straddle.Add(1, std::make_shared<EuropeanCall>(100, 1));
  straddle.Add(1, std::make_shared<EuropeanPut>(100, 1));
  const double nd1 = ch03::BlackScholesCallDelta(100, 1, 100, 0.2, 0.05);
  EXPECT_NEAR(straddle.Delta(m), 2 * nd1 - 1, 1e-7);  // O(h^2), h = 0.01
  EXPECT_NEAR(straddle.Gamma(m),
              2 * ch03::BlackScholesGamma(100, 1, 100, 0.2, 0.05), 1e-5);
  EXPECT_NEAR(straddle.Vega(m),
              2 * ch03::BlackScholesVega(100, 1, 100, 0.2, 0.05), 1e-6);
  Portfolio stock;
  stock.Add(1, std::make_shared<Stock>());
  EXPECT_NEAR(stock.Delta(m), 1, 1e-12);
  EXPECT_NEAR(stock.Gamma(m), 0, 1e-6);
}

TEST(PortfolioTest, BondDiscounts) {
  BlackScholesModel m;
  m.date = 1;
  EXPECT_NEAR(ZeroCouponBond(100, 3).Price(m), 100 * std::exp(-0.1), 1e-12);
  EXPECT_THROW(ZeroCouponBond(100, 0.5).Price(m), std::invalid_argument);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch13
