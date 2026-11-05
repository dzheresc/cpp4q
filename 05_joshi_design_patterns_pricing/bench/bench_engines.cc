// Chapters 7-9: the exotics engine, the binomial tree and implied vols.

#include <benchmark/benchmark.h>

#include <vector>

#include "ch07_exotics_engine/exotic_engine.h"
#include "ch08_trees/tree.h"
#include "ch09_solvers_implied_vol/solvers.h"

namespace {

namespace j = cpp4q::joshi;

std::vector<double> Times(std::int64_t n) {
  std::vector<double> t;
  for (std::int64_t i = 1; i <= n; ++i) {
    t.push_back(static_cast<double>(i) / static_cast<double>(n));
  }
  return t;
}

void RunEngine(benchmark::State& state, const j::ch07::PathDependent& product,
               std::uint64_t paths) {
  for (auto _ : state) {
    j::ch07::ExoticBsEngine engine(product, 0.05, 0.0, 0.2,
                                   j::ch06::RandomParkMiller(1, 1), 100);
    j::ch05::StatisticsMean mean;
    engine.DoSimulation(mean, paths);
    benchmark::DoNotOptimize(mean.GetResultsSoFar());
  }
  state.SetItemsProcessed(state.iterations() *
                          static_cast<std::int64_t>(paths) * state.range(0));
}

void BM_ExoticAsian(benchmark::State& state) {
  const j::ch07::PathDependentAsian asian(Times(state.range(0)), 1.0,
                                          j::ch04::PayOffCall(100));
  RunEngine(state, asian, 10'000);
}
BENCHMARK(BM_ExoticAsian)->Arg(12)->Arg(52)->Arg(252)->Unit(benchmark::kMillisecond);

void BM_ExoticKnockOut(benchmark::State& state) {
  const j::ch07::PathDependentKnockOut ko(Times(state.range(0)), 80,
                                          j::ch07::BarrierDirection::kDown,
                                          j::ch04::PayOffCall(100));
  RunEngine(state, ko, 10'000);
}
BENCHMARK(BM_ExoticKnockOut)->Arg(52)->Unit(benchmark::kMillisecond);

void BM_TreeEuropean(benchmark::State& state) {
  const j::ch08::TreeEuropean call(1.0, j::ch04::PayOffCall(100));
  const auto steps = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    j::ch08::SimpleBinomialTree tree(100, 0.05, 0.02, 0.2, steps, 1.0);
    benchmark::DoNotOptimize(tree.GetThePrice(call));
  }
}
BENCHMARK(BM_TreeEuropean)->Arg(100)->Arg(500)->Arg(2000)->Unit(benchmark::kMicrosecond);

void BM_TreeAmerican(benchmark::State& state) {
  const j::ch08::TreeAmerican put(1.0, j::ch04::PayOffPut(100));
  const auto steps = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    j::ch08::SimpleBinomialTree tree(100, 0.05, 0.02, 0.2, steps, 1.0);
    benchmark::DoNotOptimize(tree.GetThePrice(put));
  }
}
BENCHMARK(BM_TreeAmerican)->Arg(100)->Arg(500)->Arg(2000)->Unit(benchmark::kMicrosecond);

// Re-pricing on an already built tree (the reuse the design allows).
void BM_TreeAmericanReprice(benchmark::State& state) {
  const j::ch08::TreeAmerican put(1.0, j::ch04::PayOffPut(100));
  j::ch08::SimpleBinomialTree tree(100, 0.05, 0.02, 0.2, 2000, 1.0);
  tree.GetThePrice(put);
  for (auto _ : state) benchmark::DoNotOptimize(tree.GetThePrice(put));
}
BENCHMARK(BM_TreeAmericanReprice)->Unit(benchmark::kMicrosecond);

void BM_ImpliedVolNewton(benchmark::State& state) {
  const j::ch09::BsCall call(0.05, 0.01, 0.5, 100, 115);
  const double price = call.Price(0.31);
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch09::ImpliedVolNewton(call, price));
  }
}
BENCHMARK(BM_ImpliedVolNewton);

void BM_ImpliedVolBisection(benchmark::State& state) {
  const j::ch09::BsCall call(0.05, 0.01, 0.5, 100, 115);
  const double price = call.Price(0.31);
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch09::ImpliedVolBisection(call, price));
  }
}
BENCHMARK(BM_ImpliedVolBisection);

}  // namespace
