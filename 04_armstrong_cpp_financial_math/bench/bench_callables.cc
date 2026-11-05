// Chapters 18-21 and appendix A: callables (function pointer, virtual,
// std::function, template), implied volatility, parallel Monte Carlo,
// historical VaR, the PDE pricer and the binomial tree.

#include <benchmark/benchmark.h>

#include <cmath>
#include <functional>
#include <random>
#include <vector>

#include "appendix_a_risk_neutral/binomial.h"
#include "ch03_functions/matlib.h"
#include "ch04_flow_of_control/control.h"
#include "ch18_stl/stl.h"
#include "ch19_function_objects/solvers.h"
#include "ch20_threads/parallel_pricer.h"
#include "ch21_next_steps/pde.h"

namespace {

namespace a = cpp4q::armstrong;

constexpr int kPoints = 1 << 16;

double Integrand(double x) { return x * x * std::exp(-x); }

void BM_IntegrateFunctionPointer(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch04::IntegrateMidpoint(Integrand, 0, 10, kPoints));
  }
  state.SetItemsProcessed(state.iterations() * kPoints);
}
BENCHMARK(BM_IntegrateFunctionPointer);

void BM_IntegrateVirtual(benchmark::State& state) {
  const a::ch19::FunctionAdapter f(Integrand);
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch10::Integrate(f, 0, 10, kPoints));
  }
  state.SetItemsProcessed(state.iterations() * kPoints);
}
BENCHMARK(BM_IntegrateVirtual);

void BM_IntegrateStdFunction(benchmark::State& state) {
  const std::function<double(double)> f = Integrand;
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch19::IntegrateStdFunction(f, 0, 10, kPoints));
  }
  state.SetItemsProcessed(state.iterations() * kPoints);
}
BENCHMARK(BM_IntegrateStdFunction);

void BM_IntegrateTemplateLambda(benchmark::State& state) {
  const auto f = [](double x) { return x * x * std::exp(-x); };
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch19::IntegrateTemplate(f, 0, 10, kPoints));
  }
  state.SetItemsProcessed(state.iterations() * kPoints);
}
BENCHMARK(BM_IntegrateTemplateLambda);

void BM_ImpliedVolatility(benchmark::State& state) {
  std::vector<double> prices;
  for (int i = 0; i < 64; ++i) {
    prices.push_back(a::ch03::BlackScholesCallPrice(70 + i, 1, 100, 0.25, 0.03));
  }
  for (auto _ : state) {
    double s = 0;
    for (int i = 0; i < 64; ++i) {
      s += a::ch19::ImpliedVolatility(prices[static_cast<std::size_t>(i)], 70 + i, 1, 100, 0.03);
    }
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_ImpliedVolatility);

void BM_ParallelAsian(benchmark::State& state) {
  const a::ch20::BlackScholesModel m;
  const a::ch12::ArithmeticAsianOption asian(a::ch12::OptionType::kCall, 100,
                                             1);
  const a::ch20::ParallelSettings s{.n_scenarios = 100'000,
                                    .n_steps = 12,
                                    .n_threads = static_cast<int>(state.range(0))};
  for (auto _ : state) benchmark::DoNotOptimize(a::ch20::PriceParallel(asian, m, s));
  state.SetItemsProcessed(state.iterations() * s.n_scenarios);
}
BENCHMARK(BM_ParallelAsian)->Arg(1)->Arg(2)->Arg(4)->UseRealTime()->Unit(benchmark::kMillisecond);

void BM_HistoricalVaR(benchmark::State& state) {
  std::mt19937_64 rng(1);
  std::normal_distribution<double> z;
  std::vector<double> losses(1 << 18);
  for (double& x : losses) x = z(rng);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch18::HistoricalVaR(losses, 0.99));
  state.SetItemsProcessed(state.iterations() * (1 << 18));
}
BENCHMARK(BM_HistoricalVaR)->Unit(benchmark::kMicrosecond);

void BM_PdeEuropean(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch21::PricePde(
        a::ch21::PayoffType::kPut, a::ch21::ExerciseStyle::kEuropean, 100, 1,
        100, 0.2, 0.05, {.n_space = n, .n_time = n}));
  }
}
BENCHMARK(BM_PdeEuropean)->Arg(200)->Arg(800)->Unit(benchmark::kMillisecond);

void BM_PdeAmerican(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::ch21::PricePde(
        a::ch21::PayoffType::kPut, a::ch21::ExerciseStyle::kAmerican, 100, 1,
        100, 0.2, 0.05, {.n_space = n, .n_time = n}));
  }
}
BENCHMARK(BM_PdeAmerican)->Arg(200)->Arg(800)->Unit(benchmark::kMillisecond);

void BM_CrrAmerican(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(a::appendix_a::CrrPrice(
        a::appendix_a::PayoffType::kPut, a::appendix_a::ExerciseStyle::kAmerican,
        100, 1, 100, 0.2, 0.05, n));
  }
}
BENCHMARK(BM_CrrAmerican)->Arg(500)->Arg(2000)->Unit(benchmark::kMillisecond);

}  // namespace
