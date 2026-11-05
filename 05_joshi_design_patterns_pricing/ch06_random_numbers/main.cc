// Chapter 6 demo: plain, antithetic and quasi-random generators feeding the
// same Monte Carlo routine.

#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch06_random_numbers/random.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c4 = j::ch04;
  namespace c5 = j::ch05;
  namespace c6 = j::ch06;
  const c4::VanillaOption call(c4::PayOffCall(100), 1.0);
  const double exact = j::BlackScholesCall(100, 100, 0.05, 0, 0.2, 1);
  std::printf("Black-Scholes %.6f\n", exact);
  c6::RandomParkMiller park_miller(1);
  c6::AntiThetic antithetic{j::Wrapper<c6::RandomBase>(c6::RandomParkMiller(1))};
  c6::RandomHalton halton(1);
  const struct {
    const char* name;
    c6::RandomBase* gen;
  } gens[] = {{"Park-Miller", &park_miller},
              {"antithetic", &antithetic},
              {"Halton", &halton}};
  for (const auto& g : gens) {
    c5::StatisticsMoments stats;
    c6::SimpleMonteCarlo6(call, 100, 0.2, 0.05, 1 << 16, stats, *g.gen);
    const auto row = stats.GetResultsSoFar()[0];
    std::printf("%-12s %.6f (error %+.6f, naive std err %.6f)\n", g.name,
                row[0], row[0] - exact, row[1]);
  }
}
