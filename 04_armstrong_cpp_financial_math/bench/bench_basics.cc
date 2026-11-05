// Chapters 2-5: normal distribution functions, summation, random vectors
// and percentiles.

#include <benchmark/benchmark.h>

#include <vector>

#include "ch02_data_types/numerics.h"
#include "ch03_functions/matlib.h"
#include "ch05_multiple_files/vectors.h"

namespace {

namespace a = cpp4q::armstrong;

constexpr int kBatch = 4096;

std::vector<double> Grid(double lo, double hi) {
  std::vector<double> x(kBatch);
  for (int i = 0; i < kBatch; ++i) x[static_cast<std::size_t>(i)] = lo + (hi - lo) * (i + 0.5) / kBatch;
  return x;
}

void BM_NormCdf(benchmark::State& state) {
  const auto x = Grid(-4, 4);
  for (auto _ : state) {
    double s = 0;
    for (double v : x) s += a::ch03::NormCdf(v);
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_NormCdf);

void BM_NormCdfPolynomial(benchmark::State& state) {
  const auto x = Grid(-4, 4);
  for (auto _ : state) {
    double s = 0;
    for (double v : x) s += a::ch03::NormCdfPolynomial(v);
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_NormCdfPolynomial);

void BM_NormInv(benchmark::State& state) {
  const auto p = Grid(0, 1);
  for (auto _ : state) {
    double s = 0;
    for (double v : p) s += a::ch03::NormInv(v);
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_NormInv);

void BM_BlackScholesCall(benchmark::State& state) {
  const auto k = Grid(50, 150);
  for (auto _ : state) {
    double s = 0;
    for (double v : k) s += a::ch03::BlackScholesCallPrice(v, 1, 100, 0.2, 0.05);
    benchmark::DoNotOptimize(s);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_BlackScholesCall);

void BM_NaiveSum(benchmark::State& state) {
  const std::vector<double> v(1 << 16, 0.1);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch02::NaiveSum(v));
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_NaiveSum);

void BM_CompensatedSum(benchmark::State& state) {
  const std::vector<double> v(1 << 16, 0.1);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch02::CompensatedSum(v));
  state.SetItemsProcessed(state.iterations() * (1 << 16));
}
BENCHMARK(BM_CompensatedSum);

void BM_RandN(benchmark::State& state) {
  a::ch05::Rng rng(1);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch05::RandN(kBatch, rng));
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_RandN);

void BM_Prctile(benchmark::State& state) {
  a::ch05::Rng rng(2);
  const auto z = a::ch05::RandN(static_cast<int>(state.range(0)), rng);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch05::Prctile(z, 99));
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_Prctile)->Arg(1 << 12)->Arg(1 << 18);

}  // namespace
