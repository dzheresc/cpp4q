// Chapters 2-3: thread start-up cost, parallel accumulate scaling, and the
// cost of locks under contention.

#include <benchmark/benchmark.h>

#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "ch02_managing_threads/parallel_accumulate.h"
#include "ch03_sharing_data/account.h"
#include "ch03_sharing_data/quote_cache.h"

namespace {

namespace ch02 = cpp4q::williams::ch02;
namespace ch03 = cpp4q::williams::ch03;

void BM_ThreadSpawnJoin(benchmark::State& state) {
  for (auto _ : state) {
    std::thread t([] {});
    t.join();
  }
}
BENCHMARK(BM_ThreadSpawnJoin)->UseRealTime();

void BM_ParallelAccumulate(benchmark::State& state) {
  std::vector<double> v(4'000'000);
  std::iota(v.begin(), v.end(), 0.0);
  const auto threads = static_cast<unsigned>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch02::ParallelAccumulate(v.begin(), v.end(), 0.0, 10'000, threads));
  }
  state.SetItemsProcessed(state.iterations() *
                          static_cast<std::int64_t>(v.size()));
}
BENCHMARK(BM_ParallelAccumulate)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// Each benchmark thread transfers between random pairs of 16 accounts.
std::vector<std::unique_ptr<ch03::Account>> g_accounts;
void BM_TransferContended(benchmark::State& state) {
  if (state.thread_index() == 0) {
    g_accounts.clear();
    for (int i = 0; i < 16; ++i) {
      g_accounts.push_back(std::make_unique<ch03::Account>(1'000'000'000));
    }
  }
  // benchmark synchronises threads before the timed loop starts.
  std::mt19937 rng(static_cast<unsigned>(state.thread_index()));
  std::uniform_int_distribution<std::size_t> pick(0, 15);
  for (auto _ : state) {
    const std::size_t a = pick(rng), b = pick(rng);
    benchmark::DoNotOptimize(ch03::Transfer(*g_accounts[a], *g_accounts[b], 1));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TransferContended)->ThreadRange(1, 4)->UseRealTime();

ch03::QuoteCache g_cache;
void BM_QuoteCacheRead(benchmark::State& state) {
  static const std::vector<std::string> kSymbols = [] {
    std::vector<std::string> s;
    for (int i = 0; i < 64; ++i) s.push_back("SYM" + std::to_string(i));
    for (const auto& x : s) g_cache.Update(x, {1.0, 1.1});
    return s;
  }();
  std::size_t i = static_cast<std::size_t>(state.thread_index());
  for (auto _ : state) {
    benchmark::DoNotOptimize(g_cache.Get(kSymbols[i++ & 63]));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_QuoteCacheRead)->ThreadRange(1, 4)->UseRealTime();

}  // namespace
