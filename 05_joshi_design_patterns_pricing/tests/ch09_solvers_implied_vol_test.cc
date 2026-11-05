#include <initializer_list>
#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <stdexcept>

#include "ch09_solvers_implied_vol/solvers.h"

namespace cpp4q::joshi::ch09 {
namespace {

// x^3 - c x.
struct Cubic {
  double c = 2.0;
  double Value(double x) const { return x * x * x - c * x; }
  double Derivative(double x) const { return 3 * x * x - c; }
};

TEST(SolversTest, BisectionAndNewtonOnACubic) {
  const double root = std::sqrt(2.0);
  EXPECT_NEAR(Bisection(0.0, 1.0, 2.0, 1e-13,
                        [](double x) { return x * x * x - 2 * x; }),
              root, 1e-12);
  EXPECT_NEAR((NewtonRaphson<Cubic, &Cubic::Value, &Cubic::Derivative>(
                  0.0, 2.0, 1e-14, Cubic{})),
              root, 1e-13);
  EXPECT_NEAR(NewtonRaphson(
                  5.0, 1.0, 1e-14, [](double x) { return std::exp(x); },
                  [](double x) { return std::exp(x); }),
              std::log(5.0), 1e-14);
  EXPECT_THROW(Bisection(10.0, 0.0, 1.0, 1e-8, [](double x) { return x; }),
               std::invalid_argument);
  // f' = 0 exactly at the start.
  EXPECT_THROW((NewtonRaphson<Cubic, &Cubic::Value, &Cubic::Derivative>(
                   1.0, 0.0, 1e-14, Cubic{0.0})),
               std::runtime_error);
  // x^2 = -1 has no root: Newton wanders and gives up.
  EXPECT_THROW(NewtonRaphson(
                   -1.0, 0.5, 1e-14, [](double x) { return x * x; },
                   [](double x) { return 2 * x; }),
               std::runtime_error);
}

TEST(ImpliedVolTest, RoundTripsAcrossStrikesAndMaturities) {
  for (const double t : {0.1, 1.0, 5.0}) {
    for (const double k : {50.0, 80.0, 100.0, 125.0, 200.0}) {
      for (const double vol : {0.05, 0.3, 1.2}) {
        const BsCall call(0.04, 0.01, t, 100, k);
        const double price = call.Price(vol);
        if (price - call.Price(0.0) < 1e-8) continue;  // no time value
        EXPECT_NEAR(ImpliedVolNewton(call, price, 1e-12), vol, 1e-6)
            << t << " " << k << " " << vol;
        EXPECT_NEAR(ImpliedVolBisection(call, price, 1e-12), vol, 1e-6);
      }
    }
  }
}

TEST(ImpliedVolTest, RandomisedNewtonNeverFails) {
  std::mt19937_64 rng(9);
  std::uniform_real_distribution<double> strike(30, 300), expiry(0.02, 10),
      vol(0.02, 2), rate(-0.02, 0.1);
  int tested = 0;
  for (int i = 0; i < 2000; ++i) {
    const BsCall call(rate(rng), 0.0, expiry(rng), 100, strike(rng));
    const double v = vol(rng);
    const double price = call.Price(v);
    if (price - call.Price(0.0) < 1e-6) continue;
    ++tested;
    EXPECT_NEAR(ImpliedVolNewton(call, price, 1e-12), v, 1e-5);
  }
  EXPECT_GT(tested, 1500);
}

TEST(ImpliedVolTest, ArbitrageViolationsRejected) {
  const BsCall call(0.05, 0, 1, 100, 100);
  EXPECT_THROW(ImpliedVolNewton(call, 0.1), std::domain_error);
  EXPECT_THROW(ImpliedVolBisection(call, 101), std::domain_error);
}

}  // namespace
}  // namespace cpp4q::joshi::ch09
