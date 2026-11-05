#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>

#include "ch03_functions/matlib.h"
#include "ch04_flow_of_control/control.h"

namespace cpp4q::armstrong::ch04 {
namespace {

TEST(LoopsTest, FactorialAndPrimes) {
  EXPECT_EQ(Factorial(0), 1u);
  EXPECT_EQ(Factorial(20), 2432902008176640000u);
  EXPECT_THROW(Factorial(21), std::out_of_range);
  EXPECT_FALSE(IsPrime(1));
  EXPECT_TRUE(IsPrime(2));
  EXPECT_TRUE(IsPrime(1'000'000'007));
  EXPECT_FALSE(IsPrime(1'000'000'007ull * 3));
  const auto primes = PrimesUpTo(100);
  EXPECT_EQ(primes.size(), 25u);
  for (auto p : primes) EXPECT_TRUE(IsPrime(p));
  EXPECT_TRUE(PrimesUpTo(1).empty());
}

TEST(LoopsTest, ExpSeries) {
  for (double x : {-30.0, -1.0, 0.0, 0.5, 10.0, 50.0}) {
    EXPECT_NEAR(ExpBySeries(x) / std::exp(x), 1.0, 1e-14) << x;
  }
}

TEST(SwitchTest, Annuities) {
  const double r = 0.05;
  EXPECT_NEAR(AnnuityPresentValue(AnnuityType::kImmediate, r, 10),
              (1 - std::pow(1 + r, -10)) / r, 1e-12);
  EXPECT_NEAR(AnnuityPresentValue(AnnuityType::kDue, r, 10),
              (1 + r) * (1 - std::pow(1 + r, -10)) / r, 1e-12);
  EXPECT_EQ(AnnuityPresentValue(AnnuityType::kDue, r, 0), 0);
}

TEST(BisectionTest, FindsRoots) {
  EXPECT_NEAR(Bisection([](double x) { return x * x - 2; }, 0, 2),
              std::sqrt(2.0), 1e-12);
  EXPECT_NEAR(Bisection([](double x) { return std::cos(x) - x; }, 0, 1),
              0.7390851332151607, 1e-12);
  // tol below the spacing of doubles must still terminate.
  EXPECT_NEAR(Bisection([](double x) { return x - 1e6; }, 0, 1e7, 0), 1e6,
              1e-9);
  EXPECT_THROW(Bisection([](double x) { return x * x + 1; }, -1, 1),
               std::invalid_argument);
}

TEST(IntegrationTest, OrdersOfConvergence) {
  const auto f = [](double x) { return std::exp(x); };
  const double exact = std::exp(1.0) - 1;
  const double m1 = IntegrateMidpoint(f, 0, 1, 100) - exact;
  const double m2 = IntegrateMidpoint(f, 0, 1, 200) - exact;
  EXPECT_NEAR(m1 / m2, 4.0, 0.01);  // second order
  const double t1 = IntegrateTrapezium(f, 0, 1, 100) - exact;
  EXPECT_NEAR(t1 / m1, -2.0, 0.01);  // opposite sign, twice the size
}

TEST(IntegrationTest, CallPriceMatchesBlackScholes) {
  for (double k : {80.0, 100.0, 125.0}) {
    EXPECT_NEAR(CallPriceByIntegration(k, 1.5, 100, 0.25, 0.03, 4000),
                ch03::BlackScholesCallPrice(k, 1.5, 100, 0.25, 0.03), 1e-5)
        << k;
  }
}

}  // namespace
}  // namespace cpp4q::armstrong::ch04
