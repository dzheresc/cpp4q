#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "ch06_random_concurrency/monte_carlo.h"

namespace cpp4q::hanson::ch06 {
namespace {

constexpr GbmParams kGbm{.spot = 100.0,
                         .rate = 0.05,
                         .dividend_yield = 0.0,
                         .volatility = 0.2,
                         .expiry = 1.0};
constexpr double kBsCall = 10.450583572185565;
constexpr double kBsPut = 5.573526022256971;

TEST(MonteCarloTest, EuropeanConvergesToBlackScholes) {
  const McConfig cfg{.num_paths = 400'000, .seed = 11};
  const auto call = PriceEuropeanMc(OptionType::kCall, 100.0, kGbm, cfg);
  const auto put = PriceEuropeanMc(OptionType::kPut, 100.0, kGbm, cfg);
  EXPECT_NEAR(call.price, kBsCall, 4.0 * call.std_error);
  EXPECT_NEAR(put.price, kBsPut, 4.0 * put.std_error);
  EXPECT_EQ(call.num_samples, cfg.num_paths);
}

TEST(MonteCarloTest, SameSeedSameResult) {
  const McConfig cfg{.num_paths = 10'000, .seed = 3};
  EXPECT_EQ(PriceEuropeanMc(OptionType::kCall, 95.0, kGbm, cfg).price,
            PriceEuropeanMc(OptionType::kCall, 95.0, kGbm, cfg).price);
}

TEST(MonteCarloTest, AntitheticReducesError) {
  // Same number of normal draws for both estimators.
  const auto plain =
      PriceEuropeanMc(OptionType::kCall, 100.0, kGbm,
                      {.num_paths = 200'000, .seed = 5, .antithetic = false});
  const auto anti =
      PriceEuropeanMc(OptionType::kCall, 100.0, kGbm,
                      {.num_paths = 100'000, .seed = 5, .antithetic = true});
  EXPECT_LT(anti.std_error, plain.std_error);
}

TEST(MonteCarloTest, ParallelAgreesAndIsDeterministic) {
  const McConfig cfg{.num_paths = 400'003, .seed = 17};
  const auto a =
      PriceEuropeanMcParallel(OptionType::kCall, 100.0, kGbm, cfg, 4);
  const auto b =
      PriceEuropeanMcParallel(OptionType::kCall, 100.0, kGbm, cfg, 4);
  EXPECT_EQ(a.price, b.price);
  EXPECT_EQ(a.num_samples, cfg.num_paths);
  EXPECT_NEAR(a.price, kBsCall, 4.0 * a.std_error);
}

TEST(MonteCarloTest, AsianBelowEuropeanAndSingleStepIsEuropean) {
  const McConfig cfg{.num_paths = 100'000, .seed = 9};
  const auto asian = PriceAsianMc(OptionType::kCall, 100.0, kGbm, 52, cfg);
  EXPECT_LT(asian.price, kBsCall);
  // Known value for this setup is about 5.78 (arithmetic, 52 fixings).
  EXPECT_NEAR(asian.price, 5.78, 0.1);
  const auto one = PriceAsianMc(OptionType::kCall, 100.0, kGbm, 1, cfg);
  EXPECT_NEAR(one.price, kBsCall, 4.0 * one.std_error);
  const auto par =
      PriceAsianMcParallel(OptionType::kCall, 100.0, kGbm, 52, cfg, 3);
  EXPECT_NEAR(par.price, asian.price, 4.0 * asian.std_error);
}

TEST(MonteCarloTest, Validation) {
  GbmParams bad = kGbm;
  bad.spot = -1.0;
  EXPECT_THROW((void)PriceEuropeanMc(OptionType::kCall, 100.0, bad, {}),
               std::invalid_argument);
  EXPECT_THROW((void)PriceAsianMc(OptionType::kCall, 100.0, kGbm, 0, {}),
               std::invalid_argument);
}

TEST(SampleStatsTest, MergeEqualsSequential) {
  SampleStats a, b, all;
  for (int i = 0; i < 10; ++i) {
    (i < 4 ? a : b).Add(i);
    all.Add(i);
  }
  a.Merge(b);
  EXPECT_EQ(a.count(), all.count());
  EXPECT_DOUBLE_EQ(a.Mean(), all.Mean());
  EXPECT_DOUBLE_EQ(a.StdError(), all.StdError());
}

}  // namespace
}  // namespace cpp4q::hanson::ch06
