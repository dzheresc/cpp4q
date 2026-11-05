// Chapter 5: spin lock vs std::mutex, seqlock reads, atomic_ref histogram.

#include <benchmark/benchmark.h>

#include <mutex>
#include <random>
#include <vector>

#include "ch05_memory_model/atomic_stats.h"
#include "ch05_memory_model/seq_lock.h"
#include "ch05_memory_model/spin_lock.h"

namespace {

namespace ch05 = cpp4q::williams::ch05;

template <typename Lock>
void BM_LockedIncrement(benchmark::State& state) {
  static Lock lock;
  static long long counter = 0;
  for (auto _ : state) {
    std::lock_guard g(lock);
    benchmark::DoNotOptimize(++counter);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LockedIncrement<std::mutex>)->ThreadRange(1, 4)->UseRealTime();
BENCHMARK(BM_LockedIncrement<ch05::SpinLock>)->ThreadRange(1, 4)->UseRealTime();

ch05::SeqLock<ch05::TopOfBook> g_book;
void BM_SeqLockLoad(benchmark::State& state) {
  for (auto _ : state) benchmark::DoNotOptimize(g_book.Load());
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SeqLockLoad)->ThreadRange(1, 4)->UseRealTime();

void BM_SeqLockStore(benchmark::State& state) {
  std::int64_t i = 0;
  for (auto _ : state) {
    ++i;
    g_book.Store({100.0, 100.01, i, i});
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SeqLockStore);

std::vector<double> Pnl() {
  std::vector<double> v(4'000'000);
  std::mt19937_64 rng(3);
  std::normal_distribution<double> n(0.0, 1.0);
  for (auto& x : v) x = n(rng);
  return v;
}

void BM_HistogramSerial(benchmark::State& state) {
  static const auto pnl = Pnl();
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch05::PnlHistogram(pnl, -4.0, 4.0, 64));
  }
}
BENCHMARK(BM_HistogramSerial)->Unit(benchmark::kMillisecond)->UseRealTime();

void BM_HistogramParallel(benchmark::State& state) {
  static const auto pnl = Pnl();
  const auto threads = static_cast<unsigned>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch05::ParallelPnlHistogram(pnl, -4.0, 4.0, 64, threads));
  }
}
BENCHMARK(BM_HistogramParallel)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace
