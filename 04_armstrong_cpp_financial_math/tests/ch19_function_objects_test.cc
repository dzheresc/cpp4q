#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <stdexcept>

#include "ch03_functions/matlib.h"
#include "ch04_flow_of_control/control.h"
#include "ch19_function_objects/solvers.h"

namespace cpp4q::armstrong::ch19 {
namespace {

double Cube(double x) { return x * x * x; }

TEST(CallablesTest, FourWaysOneAnswer) {
  const auto lambda = [](double x) { return x * x * x; };
  const double fp = ch04::IntegrateMidpoint(Cube, 0, 2, 500);
  EXPECT_DOUBLE_EQ(IntegrateTemplate(lambda, 0, 2, 500), fp);
  EXPECT_DOUBLE_EQ(IntegrateStdFunction(lambda, 0, 2, 500), fp);
  EXPECT_DOUBLE_EQ(ch10::Integrate(FunctionAdapter(lambda), 0, 2, 500), fp);
  EXPECT_NEAR(fp, 4.0, 1e-4);
}

TEST(CallablesTest, FunctorsAndCaptures) {
  const CallPayoff payoff(100);
  EXPECT_EQ(payoff(120), 20);
  EXPECT_EQ(payoff(80), 0);
  std::function<double(double)> f = payoff;  // functor into std::function
  EXPECT_EQ(f(105), 5);
  const auto df = MakeDiscountFunction(0.05);
  EXPECT_DOUBLE_EQ(df(2), std::exp(-0.1));
  int calls = 0;
  const auto counted = [&calls](double x) {
    ++calls;
    return x;
  };
  IntegrateTemplate(counted, 0, 1, 37);
  EXPECT_EQ(calls, 37);  // captured by reference, so the count is visible
}

TEST(RootsTest, BisectAndNewton) {
  const auto r = Bisect([](double x) { return x * x - 2; }, 0, 2);
  EXPECT_TRUE(r.converged);
  EXPECT_NEAR(r.root, std::sqrt(2.0), 1e-12);
  EXPECT_THROW(Bisect([](double x) { return x * x + 1; }, -1, 1),
               std::invalid_argument);
  const auto n = NewtonRaphson([](double x) { return x * x - 2; },
                               [](double x) { return 2 * x; }, 1.0);
  EXPECT_TRUE(n.converged);
  EXPECT_NEAR(n.root, std::sqrt(2.0), 1e-15);
  EXPECT_LT(n.iterations, 8);  // quadratic convergence
  const auto flat = NewtonRaphson([](double) { return 1.0; },
                                  [](double) { return 0.0; }, 1.0);
  EXPECT_FALSE(flat.converged);
}

TEST(ImpliedVolTest, RecoversVolatilityAcrossStrikes) {
  for (double k : {50.0, 80.0, 100.0, 130.0, 200.0}) {
    for (double vol : {0.05, 0.2, 0.6, 1.5}) {
      const double price = ch03::BlackScholesCallPrice(k, 0.5, 100, vol, 0.03);
      // With almost no time value (price at a no-arbitrage bound) the
      // volatility is not identifiable.
      const double lower = std::max(100 - k * std::exp(-0.015), 0.0);
      if (price - lower < 1e-8) continue;
      EXPECT_NEAR(ImpliedVolatility(price, k, 0.5, 100, 0.03), vol, 1e-7)
          << k << " " << vol;
    }
  }
}

TEST(ImpliedVolTest, RejectsArbitrage) {
  EXPECT_THROW(ImpliedVolatility(101, 100, 1, 100, 0.05), std::domain_error);
  EXPECT_THROW(ImpliedVolatility(4, 100, 1, 100, 0.05), std::domain_error);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch19
