// Chapter 8 demo: a Bermudan put by binomial tree, Longstaff-Schwartz
// (in-sample high, out-of-sample low) and the random-tree bracket.

#include <cstdio>

#include "ch01_foundations/black_scholes.h"
#include "ch08_american_options/lattice.h"
#include "ch08_american_options/lsm.h"
#include "ch08_american_options/random_tree.h"

namespace g = cpp4q::glasserman;
using g::ch08::ExerciseStyle;

int main() {
  const g::ch01::BlackScholesInputs mk{.spot = 100.0,
                                       .strike = 110.0,
                                       .rate = 0.06,
                                       .vol = 0.25,
                                       .maturity = 1.0};
  const auto put = g::ch01::OptionType::kPut;
  std::printf("Put S=100 K=110 r=6%% vol=25%% T=1\n");
  std::printf("  European (Black-Scholes)  %.4f\n",
              g::ch01::BlackScholesPrice(put, mk));
  std::printf("  American (CRR, 2000)      %.4f\n",
              g::ch08::CrrPrice(mk, put, 2000, ExerciseStyle::kAmerican));
  for (std::size_t dates : {4u, 10u, 50u}) {
    const g::ch08::BermudanSpec spec{mk, put, dates};
    g::ch08::LsmPricer lsm(spec, 4);
    const auto in_sample = lsm.Train(100'000, 1);
    const auto out_sample = lsm.Price(100'000, 2);
    std::printf("\nBermudan, %zu dates: CRR %.4f\n", dates,
                g::ch08::CrrPrice(mk, put, dates * 100,
                                  ExerciseStyle::kBermudan, dates));
    std::printf("  LSM in-sample     %.4f +- %.4f\n", in_sample.mean,
                in_sample.HalfWidth());
    std::printf("  LSM out-of-sample %.4f +- %.4f\n", out_sample.mean,
                out_sample.HalfWidth());
  }
  const g::ch08::BermudanSpec spec3{mk, put, 3};
  const auto tree = g::ch08::RandomTreePrice(spec3, 50, 100, 3);
  std::printf(
      "\nBermudan, 3 dates: CRR %.4f; random tree b=50: "
      "low %.4f +- %.4f, high %.4f +- %.4f\n",
      g::ch08::CrrPrice(mk, put, 3000, ExerciseStyle::kBermudan, 3),
      tree.low.mean, tree.low.HalfWidth(), tree.high.mean,
      tree.high.HalfWidth());
  return 0;
}
