// Chapters 9-14: Monte Carlo pricers, quadrature, path-dependent options,
// portfolio Greeks and the hedging simulator.

#include <benchmark/benchmark.h>

#include <memory>

#include "ch09_monte_carlo/monte_carlo.h"
#include "ch10_interfaces/interfaces.h"
#include "ch12_sophisticated_classes/path_dependent.h"
#include "ch13_portfolio/portfolio.h"
#include "ch14_delta_hedging/hedging.h"

namespace {

namespace a = cpp4q::armstrong;

constexpr int kPaths = 50'000;

void BM_Ch09CallMc(benchmark::State& state) {
  const a::ch08::BlackScholesModel m;
  const a::ch08::CallOption call(100, 1);
  const a::ch09::MonteCarloPricer pricer(kPaths);
  a::ch09::Rng rng(1);
  for (auto _ : state) benchmark::DoNotOptimize(pricer.Price(call, m, rng));
  state.SetItemsProcessed(state.iterations() * kPaths);
}
BENCHMARK(BM_Ch09CallMc)->Unit(benchmark::kMillisecond);

void BM_Ch10CallMc(benchmark::State& state) {
  const a::ch10::BlackScholesModel m;
  const a::ch10::EuropeanCall call(100, 1);
  const a::ch10::MonteCarloPricer pricer(kPaths);
  a::ch10::Rng rng(1);
  for (auto _ : state) benchmark::DoNotOptimize(pricer.Price(call, m, rng));
  state.SetItemsProcessed(state.iterations() * kPaths);
}
BENCHMARK(BM_Ch10CallMc)->Unit(benchmark::kMillisecond);

void BM_AsianMc(benchmark::State& state) {
  const a::ch10::BlackScholesModel m;
  const a::ch12::ArithmeticAsianOption asian(a::ch12::OptionType::kCall, 100,
                                             1);
  const a::ch10::MonteCarloPricer pricer(kPaths / 4,
                                         static_cast<int>(state.range(0)));
  a::ch10::Rng rng(2);
  for (auto _ : state) benchmark::DoNotOptimize(pricer.Price(asian, m, rng));
  state.SetItemsProcessed(state.iterations() * kPaths / 4 * state.range(0));
}
BENCHMARK(BM_AsianMc)->Arg(12)->Arg(52)->Arg(252)->Unit(benchmark::kMillisecond);

void BM_BarrierMc(benchmark::State& state) {
  const a::ch10::BlackScholesModel m;
  const a::ch12::UpAndOutCall uo(100, 130, 1);
  const a::ch10::MonteCarloPricer pricer(kPaths / 4, 52);
  a::ch10::Rng rng(3);
  for (auto _ : state) benchmark::DoNotOptimize(pricer.Price(uo, m, rng));
  state.SetItemsProcessed(state.iterations() * kPaths / 4 * 52);
}
BENCHMARK(BM_BarrierMc)->Unit(benchmark::kMillisecond);

void BM_PriceByIntegration(benchmark::State& state) {
  const a::ch10::BlackScholesModel m;
  const a::ch10::EuropeanCall call(100, 1);
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch10::PriceByIntegration(call, m, 4000));
  }
}
BENCHMARK(BM_PriceByIntegration)->Unit(benchmark::kMicrosecond);

void BM_PortfolioGreeks(benchmark::State& state) {
  const a::ch13::BlackScholesModel m;
  a::ch13::Portfolio p;
  for (int i = 0; i < 100; ++i) {
    p.Add(1, std::make_shared<a::ch10::EuropeanCall>(80 + 0.4 * i, 1));
  }
  for (auto _ : state) {
    benchmark::DoNotOptimize(p.Price(m) + p.Delta(m) + p.Gamma(m) + p.Vega(m));
  }
}
BENCHMARK(BM_PortfolioGreeks)->Unit(benchmark::kMicrosecond);

void BM_DeltaHedging(benchmark::State& state) {
  const a::ch14::BlackScholesModel world;
  const a::ch14::EuropeanCall call(100, 1);
  const a::ch14::HedgingSimulator sim(world, world, call,
                                      static_cast<int>(state.range(0)));
  a::ch14::Rng rng(4);
  for (auto _ : state) benchmark::DoNotOptimize(sim.RunSimulations(1000, rng));
  state.SetItemsProcessed(state.iterations() * 1000 * state.range(0));
}
BENCHMARK(BM_DeltaHedging)->Arg(52)->Arg(252)->Unit(benchmark::kMillisecond);

}  // namespace
