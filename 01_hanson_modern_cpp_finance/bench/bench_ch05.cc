// Chapter 5: algorithms over a long price series.

#include <benchmark/benchmark.h>

#include <cmath>
#include <random>
#include <vector>

#include "ch05_algorithms_ranges/return_stats.h"

namespace {

namespace ch05 = cpp4q::hanson::ch05;

std::vector<double> Prices(std::size_t n) {
  std::mt19937_64 rng(3);
  std::normal_distribution<double> z(0.0, 0.01);
  std::vector<double> p(n);
  double s = 100.0;
  for (auto& x : p) x = (s *= std::exp(z(rng)));
  return p;
}

void BM_SimpleReturns(benchmark::State& state) {
  const auto p = Prices(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(ch05::SimpleReturns(p));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SimpleReturns)->Range(1 << 10, 1 << 20);

void BM_SampleVariance(benchmark::State& state) {
  const auto p = Prices(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(ch05::SampleVariance(p));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_SampleVariance)->Range(1 << 10, 1 << 20);

void BM_MovingAverage(benchmark::State& state) {
  const auto p = Prices(100'000);
  const auto window = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) benchmark::DoNotOptimize(ch05::MovingAverage(p, window));
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_MovingAverage)->Arg(20)->Arg(200)->Arg(2000);

void BM_MovingAverageRanges(benchmark::State& state) {
  const auto p = Prices(100'000);
  const auto window = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch05::MovingAverageRanges(p, window));
  }
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_MovingAverageRanges)->Arg(20)->Arg(200)->Arg(2000);

void BM_MaxDrawdown(benchmark::State& state) {
  const auto p = Prices(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) benchmark::DoNotOptimize(ch05::MaxDrawdown(p));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MaxDrawdown)->Range(1 << 10, 1 << 20);

void BM_HistoricalVar(benchmark::State& state) {
  const auto r =
      ch05::SimpleReturns(Prices(static_cast<std::size_t>(state.range(0))));
  for (auto _ : state) benchmark::DoNotOptimize(ch05::HistoricalVar(r, 0.99));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_HistoricalVar)->Range(1 << 10, 1 << 20);

}  // namespace
