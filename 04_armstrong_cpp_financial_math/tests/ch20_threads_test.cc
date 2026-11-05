#include <gtest/gtest.h>

#include <cmath>
#include <initializer_list>
#include <stdexcept>

#include "ch20_threads/parallel_pricer.h"

namespace cpp4q::armstrong::ch20 {
namespace {

using ch12::GeometricAsianOption;
using ch12::OptionType;

TEST(ParallelTest, BitIdenticalForAnyThreadCount) {
  const BlackScholesModel m;
  const GeometricAsianOption geo(OptionType::kCall, 100, 1);
  ParallelSettings s{.n_scenarios = 20'000, .n_steps = 12, .n_blocks = 16};
  s.n_threads = 1;
  const Estimate one = PriceParallel(geo, m, s);
  for (int t : {2, 3, 4, 16, 32}) {
    s.n_threads = t;
    const Estimate e = PriceParallel(geo, m, s);
    EXPECT_EQ(e.price, one.price) << t;
    EXPECT_EQ(e.std_error, one.std_error) << t;
    EXPECT_EQ(PriceWithAsync(geo, m, s).price, one.price) << t;
    EXPECT_NEAR(PriceWithMutex(geo, m, s).price, one.price, 1e-12) << t;
  }
}

TEST(ParallelTest, MatchesClosedForm) {
  const BlackScholesModel m;
  const GeometricAsianOption geo(OptionType::kCall, 100, 1);
  const ParallelSettings s{.n_scenarios = 200'000, .n_steps = 12};
  const Estimate e = PriceParallel(geo, m, s);
  EXPECT_TRUE(e.Contains(ch12::GeometricAsianCallPrice(m, 100, 1, 12)))
      << e.price;
  const ch10::EuropeanCall call(100, 1);
  EXPECT_TRUE(PriceParallel(call, m, s).Contains(call.Price(m)));
}

TEST(ParallelTest, SeedsChangeTheAnswerAndSettingsAreChecked) {
  const BlackScholesModel m;
  const ch10::EuropeanCall call(100, 1);
  ParallelSettings s{.n_scenarios = 1000};
  const double a = PriceParallel(call, m, s).price;
  s.seed = 2;
  EXPECT_NE(PriceParallel(call, m, s).price, a);
  s.n_threads = 0;
  EXPECT_THROW(PriceParallel(call, m, s), std::invalid_argument);
  s = {.n_scenarios = 10, .n_blocks = 64};  // fewer paths than blocks
  EXPECT_TRUE(std::isfinite(PriceParallel(call, m, s).price));
}

}  // namespace
}  // namespace cpp4q::armstrong::ch20
