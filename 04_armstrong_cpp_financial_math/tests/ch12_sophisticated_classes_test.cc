#include <gtest/gtest.h>

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "ch12_sophisticated_classes/path_dependent.h"

namespace cpp4q::armstrong::ch12 {
namespace {

using ch10::EuropeanCall;
using ch10::MonteCarloPricer;
using ch10::Rng;

TEST(BarrierTest, Payoffs) {
  const std::vector<double> touches{100, 125, 110};
  const std::vector<double> stays{100, 115, 110};
  const UpAndOutCall uo(100, 120, 1);
  const UpAndInCall ui(100, 120, 1);
  EXPECT_EQ(uo.Payoff(touches), 0);
  EXPECT_EQ(uo.Payoff(stays), 10);
  EXPECT_EQ(ui.Payoff(touches), 10);
  EXPECT_EQ(ui.Payoff(stays), 0);
  const DownAndOutPut dop(100, 85, 1);
  const DownAndInPut dip(100, 85, 1);
  const std::vector<double> dips{95, 80, 90};
  EXPECT_EQ(dop.Payoff(dips), 0);
  EXPECT_EQ(dip.Payoff(dips), 10);
  EXPECT_TRUE(uo.IsPathDependent());
  EXPECT_THROW(UpAndOutCall(100, 0, 1), std::invalid_argument);
}

TEST(BarrierTest, InPlusOutEqualsVanillaPathByPath) {
  const BlackScholesModel m;
  const MonteCarloPricer pricer(20'000, 50);
  Rng a(1), b(1), c(1);
  const double out = pricer.Price(UpAndOutCall(100, 120, 1), m, a).price;
  const double in = pricer.Price(UpAndInCall(100, 120, 1), m, b).price;
  // The vanilla must see the same 50-step paths, so price it as a
  // (never-breached) knock-in with barrier 0+: every path is "in".
  const double vanilla = pricer.Price(UpAndInCall(100, 1e-9, 1), m, c).price;
  EXPECT_NEAR(in + out, vanilla, 1e-10);
}

TEST(BarrierTest, ClosedFormsAgreeWithKnownLimits) {
  const BlackScholesModel m;
  const double call = EuropeanCall(100, 1).Price(m);
  EXPECT_NEAR(UpAndInCallPrice(m, 100, 130, 1) +
                  UpAndOutCallPrice(m, 100, 130, 1),
              call, 1e-12);
  EXPECT_NEAR(UpAndOutCallPrice(m, 100, 1e6, 1), call, 1e-10);
  EXPECT_EQ(UpAndInCallPrice(m, 100, 90, 1), call);   // H <= K
  BlackScholesModel above = m;
  above.stock_price = 135;
  EXPECT_EQ(UpAndOutCallPrice(above, 100, 130, 1), 0.0);
}

TEST(BarrierTest, DiscreteMonitoringMatchesShiftedClosedForm) {
  const BlackScholesModel m;
  constexpr int kSteps = 50;
  const MonteCarloPricer pricer(200'000, kSteps);
  Rng rng(12);
  const Estimate e = pricer.Price(UpAndOutCall(100, 130, 1), m, rng);
  const double shifted = UpAndOutCallPrice(
      m, 100, DiscreteBarrierShift(130, 0.2, 1.0 / kSteps, true), 1);
  const double continuous = UpAndOutCallPrice(m, 100, 130, 1);
  EXPECT_TRUE(e.Contains(shifted, 4.5)) << e.price << " vs " << shifted;
  EXPECT_GT(e.price, continuous + 5 * e.std_error);  // monitoring matters
}

TEST(AsianTest, PayoffsAndOrdering) {
  const std::vector<double> p{90, 100, 120};
  EXPECT_DOUBLE_EQ(
      ArithmeticAsianOption(OptionType::kCall, 100, 1).Payoff(p),
      310.0 / 3 - 100);
  EXPECT_NEAR(GeometricAsianOption(OptionType::kPut, 105, 1).Payoff(p),
              105 - std::cbrt(90.0 * 100 * 120), 1e-12);
  EXPECT_THROW(ArithmeticAsianOption(OptionType::kCall, 0, 1),
               std::invalid_argument);
}

TEST(AsianTest, GeometricMonteCarloMatchesClosedForm) {
  BlackScholesModel m;
  m.date = 0.5;
  constexpr int kFixings = 12;
  const MonteCarloPricer pricer(200'000, kFixings);
  Rng rng(13);
  const GeometricAsianOption geo(OptionType::kCall, 95, 1.5);
  const Estimate e = pricer.Price(geo, m, rng);
  const double exact = GeometricAsianCallPrice(m, 95, 1.5, kFixings);
  EXPECT_TRUE(e.Contains(exact)) << e.price << " vs " << exact;
  // One fixing is a vanilla call.
  EXPECT_NEAR(GeometricAsianCallPrice(m, 95, 1.5, 1),
              EuropeanCall(95, 1.5).Price(m), 1e-12);
  // AM-GM: arithmetic >= geometric on every path.
  Rng a(14), b(14);
  EXPECT_GT(
      pricer.Price(ArithmeticAsianOption(OptionType::kCall, 95, 1.5), m, a)
          .price,
      pricer.Price(geo, m, b).price);
}

TEST(OperatorTest, StreamsEstimate) {
  std::ostringstream out;
  out << Estimate{1.5, 0.25};
  EXPECT_EQ(out.str(), "1.5 +- 0.25");
}

}  // namespace
}  // namespace cpp4q::armstrong::ch12
