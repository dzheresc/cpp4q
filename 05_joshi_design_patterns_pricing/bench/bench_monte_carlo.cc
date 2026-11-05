// Chapters 1-6 and appendix B: Gaussian generation, the simple Monte Carlo
// routines, and the generator strategies.

#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "appendix_b_distributions/normals.h"
#include "ch01_simple_monte_carlo/simple_mc.h"
#include "ch06_random_numbers/random.h"
#include "ch11_patterns_revisited/patterns.h"

namespace {

namespace j = cpp4q::joshi;

constexpr std::uint64_t kPaths = 100'000;

template <double (*Gen)(j::Rng&)>
void BM_Gaussian(benchmark::State& state) {
  j::Rng rng(1);
  for (auto _ : state) benchmark::DoNotOptimize(Gen(rng));
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Gaussian<&j::ch01::GetOneGaussianBySummation>);
BENCHMARK(BM_Gaussian<&j::ch01::GetOneGaussianByBoxMuller>);
BENCHMARK(BM_Gaussian<&j::ch01::GetOneGaussianByPolar>);

void BM_InverseCumulativeNormal(benchmark::State& state) {
  double u = 0.0;
  for (auto _ : state) {
    u += 0.618033988749895;
    if (u >= 1.0) u -= 1.0;
    benchmark::DoNotOptimize(j::InverseCumulativeNormal(u));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_InverseCumulativeNormal);

void BM_InverseCumulativeNormalRefined(benchmark::State& state) {
  double u = 0.0;
  for (auto _ : state) {
    u += 0.618033988749895;
    if (u >= 1.0) u -= 1.0;
    benchmark::DoNotOptimize(j::InverseCumulativeNormalRefined(u));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_InverseCumulativeNormalRefined);

void BM_SimpleMonteCarlo1(benchmark::State& state) {
  j::Rng rng(1);
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        j::ch01::SimpleMonteCarlo1(1, 100, 100, 0.2, 0.05, kPaths, rng));
  }
  state.SetItemsProcessed(state.iterations() * kPaths);
}
BENCHMARK(BM_SimpleMonteCarlo1)->Unit(benchmark::kMillisecond);

void BM_SimpleMonteCarlo5(benchmark::State& state) {
  const j::ch04::VanillaOption call(j::ch04::PayOffCall(100), 1);
  j::Rng rng(1);
  for (auto _ : state) {
    j::ch05::StatisticsMean mean;
    j::ch05::SimpleMonteCarlo5(call, 100, 0.2, 0.05, kPaths, mean, rng);
    benchmark::DoNotOptimize(mean.GetResultsSoFar());
  }
  state.SetItemsProcessed(state.iterations() * kPaths);
}
BENCHMARK(BM_SimpleMonteCarlo5)->Unit(benchmark::kMillisecond);

void BM_ConvergenceTable(benchmark::State& state) {
  const j::ch04::VanillaOption call(j::ch04::PayOffCall(100), 1);
  j::Rng rng(1);
  for (auto _ : state) {
    j::ch05::ConvergenceTable table{
        j::Wrapper<j::ch05::StatisticsMc>(j::ch05::StatisticsMoments())};
    j::ch05::SimpleMonteCarlo5(call, 100, 0.2, 0.05, kPaths, table, rng);
    benchmark::DoNotOptimize(table.GetResultsSoFar());
  }
  state.SetItemsProcessed(state.iterations() * kPaths);
}
BENCHMARK(BM_ConvergenceTable)->Unit(benchmark::kMillisecond);

void RunMc6(benchmark::State& state, j::ch06::RandomBase& gen) {
  const j::ch04::VanillaOption call(j::ch04::PayOffCall(100), 1);
  for (auto _ : state) {
    j::ch05::StatisticsMean mean;
    j::ch06::SimpleMonteCarlo6(call, 100, 0.2, 0.05, kPaths, mean, gen);
    benchmark::DoNotOptimize(mean.GetResultsSoFar());
  }
  state.SetItemsProcessed(state.iterations() * kPaths);
}

void BM_Mc6ParkMiller(benchmark::State& state) {
  j::ch06::RandomParkMiller gen(1);
  RunMc6(state, gen);
}
BENCHMARK(BM_Mc6ParkMiller)->Unit(benchmark::kMillisecond);

void BM_Mc6AntiThetic(benchmark::State& state) {
  j::ch06::AntiThetic gen{
      j::Wrapper<j::ch06::RandomBase>(j::ch06::RandomParkMiller(1))};
  RunMc6(state, gen);
}
BENCHMARK(BM_Mc6AntiThetic)->Unit(benchmark::kMillisecond);

void BM_Mc6Halton(benchmark::State& state) {
  j::ch06::RandomHalton gen(1);
  RunMc6(state, gen);
}
BENCHMARK(BM_Mc6Halton)->Unit(benchmark::kMillisecond);

void BM_Mc6Mt19937(benchmark::State& state) {
  j::ch11::RandomStdAdapter<std::mt19937_64> gen(1, 1);
  RunMc6(state, gen);
}
BENCHMARK(BM_Mc6Mt19937)->Unit(benchmark::kMillisecond);

void BM_ParkMillerSkip(benchmark::State& state) {
  j::ch06::RandomParkMiller gen(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) gen.Skip(10'000);
  state.SetItemsProcessed(state.iterations() * 10'000 * state.range(0));
}
BENCHMARK(BM_ParkMillerSkip)->Arg(1)->Arg(52)->Unit(benchmark::kMicrosecond);

}  // namespace
