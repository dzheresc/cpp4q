#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <vector>

#include "ch10_interfaces/interfaces.h"

namespace cpp4q::armstrong::ch10 {
namespace {

TEST(InterfacesTest, PayoffsThroughBasePointers) {
  std::vector<std::unique_ptr<ContinuousTimeOption>> book;
  book.push_back(std::make_unique<EuropeanCall>(100, 1));
  book.push_back(std::make_unique<EuropeanPut>(100, 1));
  book.push_back(std::make_unique<DigitalCall>(100, 1));
  book.push_back(std::make_unique<DigitalPut>(100, 1));
  const std::vector<double> path{90, 120, 110};
  const std::vector<double> expected{10, 0, 1, 0};
  for (std::size_t i = 0; i < book.size(); ++i) {
    EXPECT_FALSE(book[i]->IsPathDependent());
    EXPECT_EQ(book[i]->maturity(), 1);
    EXPECT_EQ(book[i]->Payoff(path), expected[i]) << i;
  }
}

TEST(InterfacesTest, DigitalParityAndCallSpreadLimit) {
  const BlackScholesModel m;
  const DigitalCall dc(105, 2);
  const DigitalPut dp(105, 2);
  EXPECT_NEAR(dc.Price(m) + dp.Price(m), std::exp(-0.1), 1e-14);
  // A tight call spread / width converges to the digital.
  const double h = 1e-4;
  EXPECT_NEAR((EuropeanCall(105 - h, 2).Price(m) -
               EuropeanCall(105 + h, 2).Price(m)) /
                  (2 * h),
              dc.Price(m), 1e-7);
}

TEST(InterfacesTest, MonteCarloMatchesClosedForms) {
  Rng rng(10);
  BlackScholesModel m;
  m.date = 0.5;
  const MonteCarloPricer pricer(200'000);
  const EuropeanCall c(95, 1.5);
  const EuropeanPut p(95, 1.5);
  const DigitalCall dc(110, 1.5);
  const DigitalPut dp(90, 1.5);
  for (const auto* option :
       std::initializer_list<const PathIndependentOption*>{&c, &p, &dc, &dp}) {
    const auto& closed = dynamic_cast<const Priceable&>(*option);
    const Estimate e = pricer.Price(*option, m, rng);
    EXPECT_TRUE(e.Contains(closed.Price(m)))
        << e.price << " vs " << closed.Price(m);
  }
}

TEST(InterfacesTest, QuadratureMatchesClosedForms) {
  BlackScholesModel m;
  m.volatility = 0.35;
  const EuropeanCall c(90, 1);
  const EuropeanPut p(120, 1);
  // The payoff kink lies inside the interval, so the midpoint rule's
  // O(h^2) error has a larger constant than for a smooth integrand.
  EXPECT_NEAR(PriceByIntegration(c, m, 4000), c.Price(m), 5e-5);
  EXPECT_NEAR(PriceByIntegration(p, m, 4000), p.Price(m), 5e-5);
  EXPECT_NEAR(PriceByIntegration(c, m, 16000), c.Price(m), 5e-6);
  // Discontinuous payoff: first order in h, still close.
  const DigitalCall dc(100, 1);
  EXPECT_NEAR(PriceByIntegration(dc, m, 4000), dc.Price(m), 1e-3);
}

TEST(InterfacesTest, RealFunctionIntegration) {
  struct Square final : RealFunction {
    double Evaluate(double x) const override { return x * x; }
  };
  EXPECT_NEAR(Integrate(Square{}, 0, 3, 1000), 9.0, 1e-5);
  EXPECT_THROW(Integrate(Square{}, 0, 1, 0), std::invalid_argument);
}

TEST(InterfacesTest, ExpiredAndInvalid) {
  BlackScholesModel m;
  m.date = 2;
  EXPECT_THROW(EuropeanCall(100, 1).Price(m), std::invalid_argument);
  EXPECT_THROW(DigitalPut(0, 1), std::invalid_argument);
  EXPECT_DOUBLE_EQ(EuropeanCall(100, 3).Delta(m),
                   0.6368306511756191);  // tau = 1, S = K = 100
}

}  // namespace
}  // namespace cpp4q::armstrong::ch10
