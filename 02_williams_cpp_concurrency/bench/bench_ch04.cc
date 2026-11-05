// Chapter 4: queue hand-off, futures, async quicksort, barrier stepping.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>
#include <thread>
#include <vector>

#include "ch04_synchronizing/futures.h"
#include "ch04_synchronizing/thread_safe_queue.h"

namespace {

namespace ch04 = cpp4q::williams::ch04;

std::vector<double> RandomPrices(std::size_t n) {
  std::mt19937_64 rng(1);
  std::uniform_real_distribution<double> u(90.0, 110.0);
  std::vector<double> v(n);
  for (auto& x : v) x = u(rng);
  return v;
}

void BM_PricingServiceRoundTrip(benchmark::State& state) {
  ch04::PricingService service;
  for (auto _ : state) {
    benchmark::DoNotOptimize(service.Submit([] { return 1.0; }).get());
  }
}
BENCHMARK(BM_PricingServiceRoundTrip)->UseRealTime();

void BM_AsyncRoundTrip(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        std::async(std::launch::async, [] { return 1.0; }).get());
  }
}
BENCHMARK(BM_AsyncRoundTrip)->UseRealTime();

void BM_StdSort(benchmark::State& state) {
  const auto data = RandomPrices(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    auto v = data;
    std::sort(v.begin(), v.end());
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_StdSort)
    ->Arg(1 << 20)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

void BM_ParallelQuickSort(benchmark::State& state) {
  const auto data = RandomPrices(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    auto v = data;
    ch04::ParallelQuickSort(v);
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_ParallelQuickSort)
    ->Arg(1 << 20)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

void BM_LockstepMeanSpot(benchmark::State& state) {
  const auto workers = static_cast<unsigned>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch04::LockstepMeanSpot(100'000, 52, 100.0, 0.2, 1.0 / 52, workers, 1));
  }
}
BENCHMARK(BM_LockstepMeanSpot)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace
