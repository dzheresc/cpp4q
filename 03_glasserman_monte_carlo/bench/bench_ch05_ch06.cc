// Chapters 5-6: low-discrepancy generators, QMC pricing, and
// discretization schemes.

#include <benchmark/benchmark.h>

#include <vector>

#include "ch02_random_numbers/rng.h"
#include "ch05_quasi_monte_carlo/low_discrepancy.h"
#include "ch05_quasi_monte_carlo/qmc_pricing.h"
#include "ch06_discretization/schemes.h"

namespace {

namespace g = cpp4q::glasserman;

void BM_SobolNext(benchmark::State& state) {
  const auto d = static_cast<std::size_t>(state.range(0));
  g::ch05::Sobol sobol(d);
  std::vector<double> p(d);
  std::uint64_t n = 0;
  for (auto _ : state) {
    if (++n == (1u << 30)) {
      sobol.Seek(1);
      n = 1;
    }
    sobol.Next(p);
    benchmark::DoNotOptimize(p.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SobolNext)->Arg(1)->Arg(16);

void BM_HaltonNext(benchmark::State& state) {
  const auto d = static_cast<std::size_t>(state.range(0));
  g::ch05::Halton halton(d);
  std::vector<double> p(d);
  for (auto _ : state) {
    halton.Next(p);
    benchmark::DoNotOptimize(p.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HaltonNext)->Arg(1)->Arg(16);

void BM_L2StarDiscrepancy(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  constexpr std::size_t kDim = 4;
  g::ch05::Sobol sobol(kDim);
  std::vector<double> pts(n * kDim);
  for (std::size_t i = 0; i < n; ++i) {
    sobol.Next(std::span(pts).subspan(i * kDim, kDim));
  }
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch05::L2StarDiscrepancy(pts, kDim));
  }
}
BENCHMARK(BM_L2StarDiscrepancy)->Arg(1024)->Unit(benchmark::kMillisecond);

void BM_QmcAsian(benchmark::State& state) {
  const auto c = static_cast<g::ch05::PathConstruction>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch05::QmcAsianCall({}, 65'536, c, 7));
  }
  state.SetItemsProcessed(state.iterations() * 65'536);
  state.SetLabel(state.range(0) == 0 ? "random walk" : "bridge");
}
BENCHMARK(BM_QmcAsian)->Arg(0)->Arg(1)->Unit(benchmark::kMillisecond);

void BM_McAsian(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch05::McAsianCall({}, 65'536, 7));
  }
  state.SetItemsProcessed(state.iterations() * 65'536);
}
BENCHMARK(BM_McAsian)->Unit(benchmark::kMillisecond);

void BM_StrongError(benchmark::State& state) {
  const auto scheme = static_cast<g::ch06::Scheme>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        g::ch06::GbmStrongError({100, 0.05, 0.4}, 1.0, 64, scheme, 10'000, 1));
  }
  state.SetItemsProcessed(state.iterations() * 640'000);
  state.SetLabel(state.range(0) == 0 ? "Euler" : "Milstein");
}
BENCHMARK(BM_StrongError)->Arg(0)->Arg(1)->Unit(benchmark::kMillisecond);

void BM_EulerCall(benchmark::State& state) {
  const bool two_point = state.range(0) != 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        g::ch06::EulerCallPrice({}, 32, 20'000, two_point, 2));
  }
  state.SetItemsProcessed(state.iterations() * 640'000);
  state.SetLabel(two_point ? "two-point" : "normal");
}
BENCHMARK(BM_EulerCall)->Arg(0)->Arg(1)->Unit(benchmark::kMillisecond);

void BM_CirEulerFullTruncation(benchmark::State& state) {
  const g::ch03::ShortRateParams p{.kappa = 0.2, .theta = 0.04, .sigma = 0.3};
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch06::CirEulerBond(
        p, 0.02, 5.0, 20, g::ch06::CirFix::kFullTruncation, 10'000, 3));
  }
  state.SetItemsProcessed(state.iterations() * 200'000);
}
BENCHMARK(BM_CirEulerFullTruncation)->Unit(benchmark::kMillisecond);

}  // namespace
