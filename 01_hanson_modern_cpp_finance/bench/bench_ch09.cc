// Chapter 9: Boost.Math root finders and quadrature.

#include <benchmark/benchmark.h>

#include "ch09_boost/boost_pricing.h"

namespace {

namespace ch09 = cpp4q::hanson::ch09;

const ch09::BlackScholesInputs kIn{100.0, 110.0, 0.03, 0.01, 0.27, 0.75};

void BM_BlackScholesBoost(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch09::BlackScholesBoost(ch09::OptionType::kCall, kIn));
  }
}
BENCHMARK(BM_BlackScholesBoost);

void BM_ImpliedVolToms748(benchmark::State& state) {
  const double px =
      cpp4q::hanson::ch01::BlackScholesPrice(ch09::OptionType::kCall, kIn);
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch09::ImpliedVolToms748(ch09::OptionType::kCall, kIn, px));
  }
}
BENCHMARK(BM_ImpliedVolToms748);

void BM_ImpliedVolNewton(benchmark::State& state) {
  const double px =
      cpp4q::hanson::ch01::BlackScholesPrice(ch09::OptionType::kCall, kIn);
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch09::ImpliedVolNewton(ch09::OptionType::kCall, kIn, px));
  }
}
BENCHMARK(BM_ImpliedVolNewton);

void BM_PriceByQuadrature(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        ch09::PriceByQuadrature(ch09::OptionType::kCall, kIn));
  }
}
BENCHMARK(BM_PriceByQuadrature);

}  // namespace
