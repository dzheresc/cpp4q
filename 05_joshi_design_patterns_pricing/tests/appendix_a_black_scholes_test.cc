#include <initializer_list>
#include <gtest/gtest.h>

#include <cmath>

#include "appendix_a_black_scholes/black_scholes.h"

namespace cpp4q::joshi {
namespace {

constexpr double kS = 100, kR = 0.05, kD = 0.02, kVol = 0.25, kT = 1.5;

TEST(BlackScholesTest, KnownValue) {
  // Hull's textbook example: S=42, K=40, r=10%, vol=20%, T=0.5.
  EXPECT_NEAR(BlackScholesCall(42, 40, 0.1, 0, 0.2, 0.5), 4.759422392871535,
              1e-12);
  EXPECT_NEAR(BlackScholesPut(42, 40, 0.1, 0, 0.2, 0.5), 0.8085993729000922,
              1e-12);
}

TEST(BlackScholesTest, ParityRelations) {
  for (const double k : {60.0, 95.0, 100.0, 140.0}) {
    const double fwd = kS * std::exp(-kD * kT) - k * std::exp(-kR * kT);
    EXPECT_NEAR(BlackScholesCall(kS, k, kR, kD, kVol, kT) -
                    BlackScholesPut(kS, k, kR, kD, kVol, kT),
                fwd, 1e-12);
    EXPECT_NEAR(BlackScholesDigitalCall(kS, k, kR, kD, kVol, kT) +
                    BlackScholesDigitalPut(kS, k, kR, kD, kVol, kT),
                std::exp(-kR * kT), 1e-15);
  }
}

TEST(BlackScholesTest, GreeksMatchFiniteDifferences) {
  const double k = 105;
  const auto c = [&](double s, double v) {
    return BlackScholesCall(s, k, kR, kD, v, kT);
  };
  const double h = 1e-3;
  EXPECT_NEAR(BlackScholesCallDelta(kS, k, kR, kD, kVol, kT),
              (c(kS + h, kVol) - c(kS - h, kVol)) / (2 * h), 1e-8);
  EXPECT_NEAR(BlackScholesCallGamma(kS, k, kR, kD, kVol, kT),
              (c(kS + h, kVol) - 2 * c(kS, kVol) + c(kS - h, kVol)) / (h * h),
              1e-5);
  // Central-difference error h^2/6 * d3C/dvol3 ~ 3e-6 here.
  EXPECT_NEAR(BlackScholesCallVega(kS, k, kR, kD, kVol, kT),
              (c(kS, kVol + h) - c(kS, kVol - h)) / (2 * h), 1e-5);
  // A digital is minus the strike derivative of the call.
  EXPECT_NEAR(BlackScholesDigitalCall(kS, k, kR, kD, kVol, kT),
              -(BlackScholesCall(kS, k + h, kR, kD, kVol, kT) -
                BlackScholesCall(kS, k - h, kR, kD, kVol, kT)) /
                  (2 * h),
              1e-8);
}

TEST(BlackScholesTest, DegenerateInputsGiveIntrinsicForward) {
  EXPECT_NEAR(BlackScholesCall(kS, 90, kR, kD, 0.0, kT),
              kS * std::exp(-kD * kT) - 90 * std::exp(-kR * kT), 1e-12);
  EXPECT_EQ(BlackScholesPut(kS, 90, kR, kD, 0.0, kT), 0.0);
  EXPECT_EQ(BlackScholesCall(kS, 90, kR, kD, kVol, 0.0), 10.0);
  EXPECT_EQ(BlackScholesDigitalCall(kS, 90, kR, kD, kVol, 0.0), 1.0);
  EXPECT_EQ(BlackScholesCallVega(kS, 90, kR, kD, kVol, 0.0), 0.0);
}

}  // namespace
}  // namespace cpp4q::joshi
