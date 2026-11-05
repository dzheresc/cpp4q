// Chapter 2 demo: uniform generators, normal methods, non-uniform
// variates, and reproducible parallel simulation.

#include <chrono>
#include <cmath>
#include <cstdio>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"
#include "ch02_random_numbers/distributions.h"
#include "ch02_random_numbers/normal.h"
#include "ch02_random_numbers/parallel_mc.h"
#include "ch02_random_numbers/rng.h"

namespace g = cpp4q::glasserman;

int main() {
  g::ch02::Mrg32k3a rng;
  std::printf("MRG32k3a, reference seed, first uniforms:");
  for (int i = 0; i < 3; ++i) std::printf(" %.10f", rng.NextUniform());
  std::printf("\n");

  g::ch02::Randu randu(1);
  const auto a = randu.NextInt(), b = randu.NextInt(), c = randu.NextInt();
  std::printf(
      "RANDU: x3 - 6 x2 + 9 x1 = %lld (mod 2^31 always 0)\n",
      static_cast<long long>(
          ((static_cast<std::int64_t>(c) - 6LL * b + 9LL * a) % 2147483648LL +
           2147483648LL) %
          2147483648LL));

  for (double u : {0.001, 0.025, 0.5, 0.975, 0.999999}) {
    std::printf("Phi^-1(%g): BSM %.10f, refined %.12f\n", u,
                g::ch02::InverseNormalCdf(u),
                g::ch02::InverseNormalCdfAccurate(u));
  }

  g::ch02::Sampler s(g::ch02::Mrg32k3a(7));
  g::ch01::RunningStats gamma, ncx2;
  for (int i = 0; i < 100'000; ++i) {
    gamma.Add(g::ch02::SampleGamma(s, 2.5));
    ncx2.Add(g::ch02::SampleNoncentralChiSquare(s, 3.0, 4.0));
  }
  std::printf("Gamma(2.5): mean %.4f var %.4f (2.5, 2.5)\n", gamma.Mean(),
              gamma.Variance());
  std::printf("chi'^2(3, 4): mean %.4f var %.4f (7, 22)\n", ncx2.Mean(),
              ncx2.Variance());

  const g::ch01::BlackScholesInputs in{};
  const double drift = in.rate - 0.5 * in.vol * in.vol;
  auto path = [&](g::ch02::Sampler& sm) {
    const double st = in.spot * std::exp(drift + in.vol * sm.Normal());
    return std::exp(-in.rate) * std::max(st - in.strike, 0.0);
  };
  for (unsigned threads : {1u, 4u}) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto stats =
        g::ch02::ParallelSimulate(path, 4'000'000, {.threads = threads});
    const std::chrono::duration<double> dt =
        std::chrono::steady_clock::now() - t0;
    std::printf("%u thread(s): %.6f +- %.6f in %.3f s\n", threads, stats.Mean(),
                stats.StdError(), dt.count());
  }
  std::printf("Black-Scholes:  %.6f\n",
              g::ch01::BlackScholesPrice(g::ch01::OptionType::kCall, in));
  return 0;
}
