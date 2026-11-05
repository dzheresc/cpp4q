// Chapter 11 demo: a composite butterfly priced on an adapted std engine.

#include <cstdio>
#include <random>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch06_random_numbers/random.h"
#include "ch11_patterns_revisited/patterns.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c4 = j::ch04;
  namespace c11 = j::ch11;
  const double spot = 100, r = 0.05, vol = 0.2, t = 1;
  const c4::VanillaOption fly(c11::MakeButterfly(100, 10), t);
  c11::RandomStdAdapter<std::mt19937_64> gen(1, 42);
  j::ch05::StatisticsMoments stats;
  j::ch06::SimpleMonteCarlo6(fly, spot, vol, r, 1 << 18, stats, gen);
  const auto bs = [&](double k) {
    return j::BlackScholesCall(spot, k, r, 0, vol, t);
  };
  std::printf("butterfly MC %.5f +- %.5f, closed form %.5f\n",
              stats.GetResultsSoFar()[0][0], stats.GetResultsSoFar()[0][1],
              bs(90) - 2 * bs(100) + bs(110));
  const c11::PayOffCollar collar(c4::PayOffCall(100), 2, 15);
  std::printf("collar(S) at 90/105/150: %g %g %g\n", collar(90), collar(105),
              collar(150));
}
