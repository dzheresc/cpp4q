// Chapter 5 demo: discrepancy of random vs Halton vs Sobol points, and
// randomized QMC for an Asian option with random-walk vs bridge paths.

#include <cstdio>
#include <vector>

#include "ch02_random_numbers/rng.h"
#include "ch05_quasi_monte_carlo/low_discrepancy.h"
#include "ch05_quasi_monte_carlo/qmc_pricing.h"

namespace g = cpp4q::glasserman;
using g::ch05::PathConstruction;

int main() {
  constexpr std::size_t kDim = 4;
  std::printf("L2-star discrepancy in %zu dims:\n%8s %12s %12s %12s\n", kDim,
              "n", "random", "Halton", "Sobol");
  for (std::size_t n : {64u, 256u, 1024u}) {
    std::vector<double> rnd(n * kDim), hal(n * kDim), sob(n * kDim);
    g::ch02::Mrg32k3a rng(5);
    g::ch05::Halton halton(kDim);
    g::ch05::Sobol sobol(kDim);
    for (std::size_t i = 0; i < n; ++i) {
      for (std::size_t k = 0; k < kDim; ++k)
        rnd[i * kDim + k] = rng.NextUniform();
      halton.Next(std::span(hal).subspan(i * kDim, kDim));
      sobol.Next(std::span(sob).subspan(i * kDim, kDim));
    }
    std::printf("%8zu %12.6f %12.6f %12.6f\n", n,
                g::ch05::L2StarDiscrepancy(rnd, kDim),
                g::ch05::L2StarDiscrepancy(hal, kDim),
                g::ch05::L2StarDiscrepancy(sob, kDim));
  }

  const g::ch05::QmcAsianSpec spec{};
  std::printf("\nAsian call, 16 fixings, 4096 points x 32 shifts:\n");
  const auto mc = g::ch05::McAsianCall(spec, 4096 * 32, 1);
  const auto rw =
      g::ch05::RqmcAsianCall(spec, 4096, 32, PathConstruction::kRandomWalk, 1);
  const auto bb = g::ch05::RqmcAsianCall(spec, 4096, 32,
                                         PathConstruction::kBrownianBridge, 1);
  std::printf("  MC                 %.5f +- %.5f\n", mc.mean, mc.HalfWidth());
  std::printf("  RQMC random walk   %.5f +- %.5f\n", rw.mean, rw.HalfWidth());
  std::printf("  RQMC bridge        %.5f +- %.5f\n", bb.mean, bb.HalfWidth());
  return 0;
}
