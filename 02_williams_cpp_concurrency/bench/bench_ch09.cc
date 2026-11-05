// Chapter 9: thread pool task overhead and recursive sort on the pool.

#include <benchmark/benchmark.h>

#include <algorithm>
#include <future>
#include <random>
#include <vector>

#include "ch09_advanced_thread_mgmt/thread_pool.h"

namespace {

namespace ch09 = cpp4q::williams::ch09;

// Submit 10k trivial tasks and wait for all of them.
void BM_PoolSubmit10k(benchmark::State& state) {
  ch09::ThreadPool pool(4);
  std::vector<std::future<int>> fs;
  fs.reserve(10'000);
  for (auto _ : state) {
    fs.clear();
    for (int i = 0; i < 10'000; ++i)
      fs.push_back(pool.Submit([i] { return i; }));
    for (auto& f : fs) benchmark::DoNotOptimize(f.get());
  }
  state.SetItemsProcessed(state.iterations() * 10'000);
}
BENCHMARK(BM_PoolSubmit10k)->UseRealTime()->Unit(benchmark::kMillisecond);

void BM_PoolQuickSort(benchmark::State& state) {
  ch09::ThreadPool pool(4);
  std::vector<double> data(1 << 20);
  std::mt19937_64 rng(2);
  std::uniform_real_distribution<double> u(90.0, 110.0);
  for (auto& x : data) x = u(rng);
  for (auto _ : state) {
    auto v = data;
    ch09::PoolQuickSort(pool, v);
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_PoolQuickSort)->UseRealTime()->Unit(benchmark::kMillisecond);

}  // namespace
