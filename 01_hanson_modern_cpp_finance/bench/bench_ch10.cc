// Chapter 10: concept-constrained binomial tree.

#include <benchmark/benchmark.h>

#include "ch03_polymorphism/payoff.h"
#include "ch10_modules_concepts/generic_pricing.h"

namespace {

namespace ch10 = cpp4q::hanson::ch10;

void BM_Binomial(benchmark::State& state, ch10::ExerciseStyle style) {
  const ch10::TreeParams<double> p{
      100.0, 0.05, 0.0, 0.2, 1.0, static_cast<std::size_t>(state.range(0))};
  const cpp4q::hanson::ch03::Put put{100.0};
  for (auto _ : state)
    benchmark::DoNotOptimize(ch10::PriceBinomial(p, put, style));
  state.SetItemsProcessed(state.iterations() * state.range(0) * state.range(0) /
                          2);
}
BENCHMARK_CAPTURE(BM_Binomial, European, ch10::ExerciseStyle::kEuropean)
    ->RangeMultiplier(4)
    ->Range(128, 2048)
    ->Unit(benchmark::kMicrosecond);
BENCHMARK_CAPTURE(BM_Binomial, American, ch10::ExerciseStyle::kAmerican)
    ->RangeMultiplier(4)
    ->Range(128, 2048)
    ->Unit(benchmark::kMicrosecond);

void BM_BinomialVirtualPayoff(benchmark::State& state) {
  const ch10::TreeParams<double> p{
      100.0, 0.05, 0.0, 0.2, 1.0, static_cast<std::size_t>(state.range(0))};
  const cpp4q::hanson::ch03::PutPayoff put(100.0);
  const cpp4q::hanson::ch03::Payoff& base = put;  // forces virtual calls
  auto f = [&base](double s) { return base(s); };
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch10::PriceBinomial(p, f, ch10::ExerciseStyle::kAmerican));
  }
}
BENCHMARK(BM_BinomialVirtualPayoff)->Arg(2048)->Unit(benchmark::kMicrosecond);

}  // namespace
