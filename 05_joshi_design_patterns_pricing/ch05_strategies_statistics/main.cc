// Chapter 5 demo: a convergence table of the first four moments.

#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch05_strategies_statistics/statistics.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c4 = j::ch04;
  namespace c5 = j::ch05;
  const c4::VanillaOption call(c4::PayOffCall(100), 1.0);
  c5::ConvergenceTable table{j::Wrapper<c5::StatisticsMc>(c5::StatisticsMoments())};
  j::Rng rng(5);
  c5::SimpleMonteCarlo5(call, 100, 0.2, 0.05, 1 << 18, table, rng);
  std::printf("Black-Scholes %.6f\n",
              j::BlackScholesCall(100, 100, 0.05, 0, 0.2, 1));
  std::printf("%8s %10s %9s %9s %7s %7s\n", "paths", "mean", "std err",
              "variance", "skew", "x kurt");
  for (const auto& row : table.GetResultsSoFar()) {
    if (row[5] < 1024) continue;
    std::printf("%8.0f %10.5f %9.5f %9.3f %7.3f %7.3f\n", row[5], row[0],
                row[1], row[2], row[3], row[4]);
  }
}
