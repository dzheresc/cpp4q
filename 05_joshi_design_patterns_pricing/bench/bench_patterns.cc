// Chapters 4, 12, 14, 16 and appendix C: the cost of the design patterns
// themselves (dispatch, cloning, factories, pimpl, the array class).

#include <benchmark/benchmark.h>

#include <functional>
#include <vector>

#include "appendix_c_array/mj_array.h"
#include "ch12_situation_2007/modern.h"
#include "ch14_template_factory/registrations.h"
#include "ch16_decoupling/any_payoff.h"
#include "ch16_decoupling/mc_pricer.h"

namespace {

namespace j = cpp4q::joshi;

const std::vector<double>& Spots() {
  static const std::vector<double> spots = [] {
    j::Rng rng(1);
    return j::ch12::GenerateTerminalSpots({}, 1 << 16, rng);
  }();
  return spots;
}

void BM_DispatchVirtual(benchmark::State& state) {
  const j::ch04::PayOffCall call(100);
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch12::MeanPayOffVirtual(call, Spots()));
  }
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_DispatchVirtual)->Unit(benchmark::kMicrosecond);

void BM_DispatchVirtualBatched(benchmark::State& state) {
  const j::ch04::PayOffCall call(100);
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch12::MeanPayOffVirtualBatched(call, Spots()));
  }
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_DispatchVirtualBatched)->Unit(benchmark::kMicrosecond);

void BM_DispatchStdFunction(benchmark::State& state) {
  const std::function<double(double)> call = j::ch12::Call{100};
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch12::MeanPayOffFunction(call, Spots()));
  }
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_DispatchStdFunction)->Unit(benchmark::kMicrosecond);

void BM_DispatchTemplate(benchmark::State& state) {
  const j::ch12::Call call{100};
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch12::MeanPayOffTemplate(call, Spots()));
  }
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_DispatchTemplate)->Unit(benchmark::kMicrosecond);

void BM_DispatchVariant(benchmark::State& state) {
  const j::ch12::PayOffVariant call = j::ch12::Call{100};
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch12::MeanPayOffVariant(call, Spots()));
  }
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_DispatchVariant)->Unit(benchmark::kMicrosecond);

void BM_DispatchAnyPayOff(benchmark::State& state) {
  const j::ch16::AnyPayOff call = j::ch12::Call{100};
  for (auto _ : state) {
    benchmark::DoNotOptimize(j::ch12::MeanPayOffTemplate(call, Spots()));
  }
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_DispatchAnyPayOff)->Unit(benchmark::kMicrosecond);

void BM_BridgeCopy(benchmark::State& state) {
  const j::ch04::VanillaOption option(j::ch04::PayOffCall(100), 1);
  for (auto _ : state) {
    j::ch04::VanillaOption copy = option;
    benchmark::DoNotOptimize(copy);
  }
}
BENCHMARK(BM_BridgeCopy);

void BM_FactoryCreate(benchmark::State& state) {
  auto& f = j::ch14::StrikePayOffFactory::Instance();
  for (auto _ : state) benchmark::DoNotOptimize(f.Create("digital_call", 100));
}
BENCHMARK(BM_FactoryCreate);

void BM_McPricerPimpl(benchmark::State& state) {
  j::ch16::McPricer pricer("call", 100, 1);
  pricer.SetMarket(100, 0.05, 0, 0.2);
  for (auto _ : state) benchmark::DoNotOptimize(pricer.Price(100'000, 1));
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_McPricerPimpl)->Unit(benchmark::kMillisecond);

void BM_MjArrayAxpy(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  j::MjArray x(n, 1.5);
  j::MjArray y(n, 2.0);
  for (auto _ : state) {
    y += 0.5 * x;  // a temporary per call, as in the book
    benchmark::DoNotOptimize(y.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MjArrayAxpy)->Arg(64)->Arg(4096);

void BM_MjArrayAddScaled(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  j::MjArray x(n, 1.5);
  j::MjArray y(n, 2.0);
  for (auto _ : state) {
    y.AddScaled(0.5, x);  // no temporary
    benchmark::DoNotOptimize(y.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_MjArrayAddScaled)->Arg(64)->Arg(4096);

}  // namespace
