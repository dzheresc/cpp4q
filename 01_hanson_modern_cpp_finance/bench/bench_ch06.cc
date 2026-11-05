// Chapter 6: Monte Carlo throughput, serial and parallel.

#include <benchmark/benchmark.h>

#include "ch06_random_concurrency/monte_carlo.h"

namespace {

namespace ch06 = cpp4q::hanson::ch06;

constexpr ch06::GbmParams kGbm{100.0, 0.03, 0.0, 0.2, 1.0};

void BM_EuropeanMc(benchmark::State& state) {
  const ch06::McConfig cfg{.num_paths = 100'000};
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch06::PriceEuropeanMc(ch06::OptionType::kCall, 100.0, kGbm, cfg));
  }
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_EuropeanMc)->Unit(benchmark::kMillisecond);

void BM_EuropeanMcParallel(benchmark::State& state) {
  const ch06::McConfig cfg{.num_paths = 1'000'000};
  const auto threads = static_cast<unsigned>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch06::PriceEuropeanMcParallel(
        ch06::OptionType::kCall, 100.0, kGbm, cfg, threads));
  }
  state.SetItemsProcessed(state.iterations() * 1'000'000);
}
BENCHMARK(BM_EuropeanMcParallel)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

void BM_AsianMc(benchmark::State& state) {
  const ch06::McConfig cfg{.num_paths = 20'000};
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch06::PriceAsianMc(ch06::OptionType::kCall, 100.0, kGbm, 52, cfg));
  }
  state.SetItemsProcessed(state.iterations() * 20'000 * 52);
}
BENCHMARK(BM_AsianMc)->Unit(benchmark::kMillisecond);

void BM_AsianMcParallel(benchmark::State& state) {
  const ch06::McConfig cfg{.num_paths = 100'000};
  const auto threads = static_cast<unsigned>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch06::PriceAsianMcParallel(
        ch06::OptionType::kCall, 100.0, kGbm, 52, cfg, threads));
  }
  state.SetItemsProcessed(state.iterations() * 100'000 * 52);
}
BENCHMARK(BM_AsianMcParallel)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace
