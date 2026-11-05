#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch08_american_options/lattice.h"
#include "ch08_american_options/lsm.h"
#include "ch08_american_options/random_tree.h"

namespace cpp4q::glasserman::ch08 {
namespace {

const BlackScholesInputs kMarket{
    .spot = 100.0, .strike = 110.0, .rate = 0.06, .vol = 0.25, .maturity = 1.0};

TEST(CrrTest, EuropeanConvergesToBlackScholes) {
  for (auto type : {OptionType::kCall, OptionType::kPut}) {
    EXPECT_NEAR(CrrPrice(kMarket, type, 2000, ExerciseStyle::kEuropean),
                ch01::BlackScholesPrice(type, kMarket), 5e-3);
  }
}

TEST(CrrTest, ExerciseValueOrdering) {
  const double eu =
      CrrPrice(kMarket, OptionType::kPut, 1200, ExerciseStyle::kEuropean);
  const double b4 =
      CrrPrice(kMarket, OptionType::kPut, 1200, ExerciseStyle::kBermudan, 4);
  const double b12 =
      CrrPrice(kMarket, OptionType::kPut, 1200, ExerciseStyle::kBermudan, 12);
  const double am =
      CrrPrice(kMarket, OptionType::kPut, 1200, ExerciseStyle::kAmerican);
  EXPECT_LT(eu, b4);
  EXPECT_LT(b4, b12);
  EXPECT_LT(b12, am);
  // Without dividends an American call is worth the European.
  EXPECT_NEAR(
      CrrPrice(kMarket, OptionType::kCall, 1200, ExerciseStyle::kAmerican),
      CrrPrice(kMarket, OptionType::kCall, 1200, ExerciseStyle::kEuropean),
      1e-10);
}

TEST(CrrTest, RejectsBadArguments) {
  EXPECT_THROW(
      CrrPrice(kMarket, OptionType::kPut, 10, ExerciseStyle::kBermudan, 3),
      std::invalid_argument);
  EXPECT_THROW(CrrPrice(kMarket, OptionType::kPut, 0, ExerciseStyle::kAmerican),
               std::invalid_argument);
}

TEST(NormalEquationsTest, SolvesSmallSystem) {
  std::vector<double> a = {4, 2, 2, 3};
  std::vector<double> b = {10, 8};  // x = (1.75, 1.5)
  SolveNormalEquations(a, b, 2);
  EXPECT_NEAR(b[0], 1.75, 1e-12);
  EXPECT_NEAR(b[1], 1.5, 1e-12);
}

TEST(LsmTest, BracketsTheBermudanPrice) {
  const BermudanSpec spec{kMarket, OptionType::kPut, 10};
  const double exact =
      CrrPrice(kMarket, OptionType::kPut, 2000, ExerciseStyle::kBermudan, 10);
  LsmPricer lsm(spec, 4);
  const auto in_sample = lsm.Train(100'000, 1);
  const auto out_sample = lsm.Price(100'000, 2);
  EXPECT_NEAR(in_sample.mean, exact, 4 * in_sample.std_error + 0.02);
  // A sub-optimal policy can only lose value: low-biased.
  EXPECT_LT(out_sample.mean, exact + 3 * out_sample.std_error);
  EXPECT_GT(out_sample.mean, exact - 0.1);
  EXPECT_EQ(lsm.coefficients().size(), 9u);
}

TEST(LsmTest, OneDateIsEuropean) {
  const BermudanSpec spec{kMarket, OptionType::kPut, 1};
  LsmPricer lsm(spec, 3);
  const auto e = lsm.Train(200'000, 3);
  EXPECT_TRUE(
      e.Contains(ch01::BlackScholesPrice(OptionType::kPut, kMarket), 4));
  EXPECT_TRUE(
      lsm.Price(200'000, 4)
          .Contains(ch01::BlackScholesPrice(OptionType::kPut, kMarket), 4));
}

TEST(LsmTest, PriceBeforeTrainThrows) {
  const LsmPricer lsm({kMarket, OptionType::kPut, 5});
  EXPECT_THROW(lsm.Price(10, 1), std::logic_error);
  EXPECT_THROW(LsmPricer({kMarket, OptionType::kPut, 5}, 0),
               std::invalid_argument);
}

TEST(RandomTreeTest, HighAboveLowAndBracketTruth) {
  const BermudanSpec spec{kMarket, OptionType::kPut, 3};
  const double exact =
      CrrPrice(kMarket, OptionType::kPut, 3000, ExerciseStyle::kBermudan, 3);
  const auto r = RandomTreePrice(spec, 20, 400, 5);
  EXPECT_GT(r.high.mean, r.low.mean);
  EXPECT_LT(r.low.Lower(3.0), exact);
  EXPECT_GT(r.high.Upper(3.0), exact);
  EXPECT_THROW(RandomTreePrice(spec, 1, 1, 1), std::invalid_argument);
}

TEST(RandomTreeTest, OneDateIsUnbiasedEuropean) {
  const BermudanSpec spec{kMarket, OptionType::kPut, 1};
  const auto r = RandomTreePrice(spec, 50, 2000, 6);
  const double eu = ch01::BlackScholesPrice(OptionType::kPut, kMarket);
  EXPECT_TRUE(r.high.Contains(eu, 4));
  EXPECT_TRUE(r.low.Contains(eu, 4));
}

}  // namespace
}  // namespace cpp4q::glasserman::ch08
