// Chapter 8: false sharing, early-exit find, parallel scan, scenario VaR.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include "ch08_designing_concurrent_code/parallel_algorithms.h"

namespace {

namespace ch08 = cpp4q::williams::ch08;

const std::vector<std::int32_t>& Ids() {
  static const auto ids = [] {
    std::vector<std::int32_t> v(4'000'000);
    std::iota(v.begin(), v.end(), 0);
    return v;
  }();
  return ids;
}

void BM_CountPacked(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch08::CountByBucketPacked(
        Ids(), static_cast<unsigned>(state.range(0))));
  }
}
BENCHMARK(BM_CountPacked)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

void BM_CountPadded(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch08::CountByBucketPadded(
        Ids(), static_cast<unsigned>(state.range(0))));
  }
}
BENCHMARK(BM_CountPadded)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

std::vector<double> Pnl(std::size_t n) {
  std::vector<double> v(n);
  std::mt19937_64 rng(5);
  std::normal_distribution<double> d(0.0, 1.0);
  for (auto& x : v) x = d(rng);
  return v;
}

// Match at 90% of the way through: worst case for early exit.
void BM_FindSerial(benchmark::State& state) {
  auto v = Pnl(8'000'000);
  for (auto& x : v) x = std::min(x, 5.0);
  v[v.size() * 9 / 10] = 10.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        std::find_if(v.begin(), v.end(), [](double x) { return x > 9.0; }));
  }
}
BENCHMARK(BM_FindSerial)->Unit(benchmark::kMillisecond)->UseRealTime();

void BM_FindParallel(benchmark::State& state) {
  auto v = Pnl(8'000'000);
  for (auto& x : v) x = std::min(x, 5.0);
  v[v.size() * 9 / 10] = 10.0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch08::ParallelFind(
        v.begin(), v.end(), [](double x) { return x > 9.0; }, 4));
  }
}
BENCHMARK(BM_FindParallel)->Unit(benchmark::kMillisecond)->UseRealTime();

void BM_ScanSerial(benchmark::State& state) {
  const auto in = Pnl(8'000'000);
  std::vector<double> out(in.size());
  for (auto _ : state) {
    std::inclusive_scan(in.begin(), in.end(), out.begin());
    benchmark::DoNotOptimize(out.data());
  }
}
BENCHMARK(BM_ScanSerial)->Unit(benchmark::kMillisecond)->UseRealTime();

void BM_ScanParallel(benchmark::State& state) {
  const auto in = Pnl(8'000'000);
  std::vector<double> out(in.size());
  for (auto _ : state) {
    ch08::ParallelInclusiveScan<double>(in, out, 4);
    benchmark::DoNotOptimize(out.data());
  }
}
BENCHMARK(BM_ScanParallel)->Unit(benchmark::kMillisecond)->UseRealTime();

void BM_ScenarioVar(benchmark::State& state) {
  constexpr std::size_t kFactors = 50, kScen = 20'000;
  const std::vector<double> exposures(kFactors, 1e5);
  const auto scen = Pnl(kFactors * kScen);
  const auto threads = static_cast<unsigned>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch08::ScenarioVar(exposures, scen, 0.99, threads));
  }
}
BENCHMARK(BM_ScenarioVar)
    ->Arg(1)
    ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace
