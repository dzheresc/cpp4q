// This test includes only the chapter 16 headers: the firewall works if it
// compiles and links without any other project header.

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ch16_decoupling/any_payoff.h"
#include "ch16_decoupling/mc_pricer.h"

namespace cpp4q::joshi::ch16 {
namespace {

static_assert(std::is_nothrow_move_constructible_v<McPricer>);
static_assert(std::is_nothrow_move_constructible_v<AnyPayOff>);

double BsCall(double s, double k, double r, double d, double v, double t) {
  // Written out here so the test needs no other header.
  const auto n = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
  const double sd = v * std::sqrt(t);
  const double d1 = (std::log(s / k) + (r - d) * t) / sd + 0.5 * sd;
  return s * std::exp(-d * t) * n(d1) - k * std::exp(-r * t) * n(d1 - sd);
}

TEST(McPricerTest, PricesThroughTheFirewall) {
  McPricer pricer("call", 100, 1);
  pricer.SetMarket(100, 0.05, 0.03, 0.25);
  const PricingResult res = pricer.Price(200'000, 1);
  EXPECT_NEAR(res.price, BsCall(100, 100, 0.05, 0.03, 0.25, 1),
              4 * res.std_error);
  pricer.SetGenerator("halton");
  EXPECT_NEAR(pricer.Price(200'000, 0).price,
              BsCall(100, 100, 0.05, 0.03, 0.25, 1), 2e-3);
  EXPECT_THROW(pricer.SetGenerator("dice"), std::out_of_range);
  EXPECT_THROW(McPricer("bermudan", 100, 1), std::out_of_range);
}

TEST(McPricerTest, ValueSemantics) {
  McPricer a("put", 100, 1);
  a.SetMarket(100, 0.05, 0, 0.2);
  McPricer b = a;
  b.SetMarket(100, 0.05, 0, 0.4);  // does not affect a
  const double pa = a.Price(20'000, 3).price;
  EXPECT_LT(pa, b.Price(20'000, 3).price);
  McPricer c = std::move(b);
  b = a;  // a moved-from pricer can be assigned to
  EXPECT_EQ(b.Price(20'000, 3).price, pa);
  const auto names = McPricer::KnownPayOffs();
  EXPECT_FALSE(names.empty());
}

TEST(AnyPayOffTest, ValuesWithoutABaseClass) {
  struct Strangle {
    double low, high;
    double operator()(double s) const {
      return s < low ? low - s : (s > high ? s - high : 0.0);
    }
  };
  AnyPayOff p = Strangle{90, 110};
  EXPECT_EQ(p(80), 10);
  EXPECT_EQ(p(100), 0);
  AnyPayOff q = p;  // deep copy
  q = [](double s) { return s; };
  EXPECT_EQ(p(120), 10);
  EXPECT_EQ(q(120), 120);
  std::vector<AnyPayOff> book{p, q, Strangle{50, 60}};
  double total = 0;
  for (const auto& x : book) total += x(70);
  EXPECT_EQ(total, 20 + 70 + 10);
}

}  // namespace
}  // namespace cpp4q::joshi::ch16
