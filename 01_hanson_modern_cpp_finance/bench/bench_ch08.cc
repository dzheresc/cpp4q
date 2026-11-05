// Chapter 8: dense linear algebra kernels.

#include <benchmark/benchmark.h>

#include <random>
#include <valarray>

#include "ch08_linear_algebra/decompositions.h"
#include "ch08_linear_algebra/matrix.h"
#include "ch08_linear_algebra/valarray_ops.h"

namespace {

namespace ch08 = cpp4q::hanson::ch08;

ch08::Matrix Random(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  ch08::Matrix m(n, n);
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) m(i, j) = u(rng);
  }
  return m;
}

void SetFlops(benchmark::State& state, double n) {
  state.counters["GFLOP/s"] = benchmark::Counter(
      2.0 * n * n * n * static_cast<double>(state.iterations()) * 1e-9,
      benchmark::Counter::kIsRate);
}

void BM_MultiplyNaive(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const auto a = Random(n, 1);
  const auto b = Random(n, 2);
  for (auto _ : state) benchmark::DoNotOptimize(ch08::MultiplyNaive(a, b));
  SetFlops(state, static_cast<double>(n));
}
BENCHMARK(BM_MultiplyNaive)
    ->RangeMultiplier(2)
    ->Range(64, 512)
    ->Unit(benchmark::kMillisecond);

void BM_Multiply(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const auto a = Random(n, 1);
  const auto b = Random(n, 2);
  for (auto _ : state) benchmark::DoNotOptimize(ch08::Multiply(a, b));
  SetFlops(state, static_cast<double>(n));
}
BENCHMARK(BM_Multiply)
    ->RangeMultiplier(2)
    ->Range(64, 512)
    ->Unit(benchmark::kMillisecond);

void BM_MultiplyValarray(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const auto a = Random(n, 1);
  const auto b = Random(n, 2);
  const ch08::ValMatrix va{n, n, std::valarray<double>(a.data().data(), n * n)};
  const ch08::ValMatrix vb{n, n, std::valarray<double>(b.data().data(), n * n)};
  for (auto _ : state) benchmark::DoNotOptimize(ch08::MultiplyValarray(va, vb));
  SetFlops(state, static_cast<double>(n));
}
BENCHMARK(BM_MultiplyValarray)
    ->RangeMultiplier(2)
    ->Range(64, 256)
    ->Unit(benchmark::kMillisecond);

void BM_Cholesky(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const auto r = Random(n, 3);
  const auto spd = r * r.Transpose() + ch08::Matrix::Identity(n);
  for (auto _ : state) benchmark::DoNotOptimize(ch08::Cholesky(spd));
}
BENCHMARK(BM_Cholesky)
    ->RangeMultiplier(2)
    ->Range(32, 512)
    ->Unit(benchmark::kMicrosecond);

void BM_LuSolve(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const auto a = Random(n, 4) + ch08::Matrix::Identity(n) * 4.0;
  const std::vector<double> b(n, 1.0);
  for (auto _ : state) {
    const ch08::LuDecomposition lu(a);
    benchmark::DoNotOptimize(lu.Solve(b));
  }
}
BENCHMARK(BM_LuSolve)
    ->RangeMultiplier(2)
    ->Range(32, 512)
    ->Unit(benchmark::kMicrosecond);

}  // namespace
