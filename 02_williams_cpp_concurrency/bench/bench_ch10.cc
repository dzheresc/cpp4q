// Chapter 10: the same algorithm under seq / par / par_unseq.

#include <benchmark/benchmark.h>

#include <execution>
#include <vector>

#include "ch10_parallel_algorithms/portfolio_algos.h"

namespace {

namespace ch10 = cpp4q::williams::ch10;

const std::vector<ch10::Bond>& Book() {
  static const auto book = ch10::MakeBondBook(1'000'000, 1);
  return book;
}

template <typename Policy>
void BM_PortfolioPv(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch10::PortfolioPv(Policy{}, Book(), 0.04));
  }
  state.SetItemsProcessed(state.iterations() *
                          static_cast<std::int64_t>(Book().size()));
}
BENCHMARK(BM_PortfolioPv<std::execution::sequenced_policy>)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
BENCHMARK(BM_PortfolioPv<std::execution::parallel_policy>)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
BENCHMARK(BM_PortfolioPv<std::execution::parallel_unsequenced_policy>)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

template <typename Policy>
void BM_SortByNotional(benchmark::State& state) {
  for (auto _ : state) {
    auto v = Book();
    ch10::SortByNotional(Policy{}, v);
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_SortByNotional<std::execution::sequenced_policy>)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
BENCHMARK(BM_SortByNotional<std::execution::parallel_policy>)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

}  // namespace
