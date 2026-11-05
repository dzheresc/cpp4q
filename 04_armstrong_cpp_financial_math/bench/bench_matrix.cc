// Chapters 16-17: matrix product, Cholesky, LU solve, compile-time fixed
// matrices, and the correlated multi-stock model.

#include <benchmark/benchmark.h>

#include <array>
#include <random>
#include <vector>

#include "ch16_matrix/matrix.h"
#include "ch16_matrix/multi_stock.h"
#include "ch17_templates/templates.h"

namespace {

namespace a = cpp4q::armstrong;
using a::ch16::Matrix;

Matrix Random(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_real_distribution<double> u(-1, 1);
  Matrix m(n, n);
  for (std::size_t k = 0; k < m.size(); ++k) m.data()[k] = u(rng);
  return m;
}

void BM_MatrixMultiply(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const Matrix x = Random(n, 1), y = Random(n, 2);
  for (auto _ : state) benchmark::DoNotOptimize(x * y);
  state.SetItemsProcessed(state.iterations() * state.range(0) *
                          state.range(0) * state.range(0));
}
BENCHMARK(BM_MatrixMultiply)->Arg(64)->Arg(256)->Unit(benchmark::kMicrosecond);

void BM_Cholesky(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const Matrix x = Random(n, 3);
  const Matrix spd = x * x.Transpose() + Matrix::Identity(n);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch16::Cholesky(spd));
}
BENCHMARK(BM_Cholesky)->Arg(64)->Arg(256)->Unit(benchmark::kMicrosecond);

void BM_Solve(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const Matrix x = Random(n, 4);
  const std::vector<double> b(n, 1.0);
  for (auto _ : state) benchmark::DoNotOptimize(a::ch16::Solve(x, b));
}
BENCHMARK(BM_Solve)->Arg(64)->Arg(256)->Unit(benchmark::kMicrosecond);

void BM_FixedMatrix4x4(benchmark::State& state) {
  a::ch17::FixedMatrix<double, 4, 4> m(std::array<double, 16>{
      1, 0.1, 0.2, 0.3, 0.1, 1, 0.1, 0.2, 0.2, 0.1, 1, 0.1, 0.3, 0.2, 0.1, 1});
  for (auto _ : state) {
    benchmark::DoNotOptimize(m);
    m = m * m;
    m(0, 0) = 1;
  }
}
BENCHMARK(BM_FixedMatrix4x4);

void BM_DynamicMatrix4x4(benchmark::State& state) {
  Matrix m{{1, 0.1, 0.2, 0.3}, {0.1, 1, 0.1, 0.2}, {0.2, 0.1, 1, 0.1},
           {0.3, 0.2, 0.1, 1}};
  for (auto _ : state) {
    benchmark::DoNotOptimize(m.data());
    m = m * m;
    m(0, 0) = 1;
  }
}
BENCHMARK(BM_DynamicMatrix4x4);

void BM_BasketMc(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  Matrix rho(n, n, 0.3);
  for (std::size_t i = 0; i < n; ++i) rho(i, i) = 1;
  const a::ch16::MultiStockModel model(std::vector<double>(n, 100),
                                       std::vector<double>(n, 0.2), rho, 0.05);
  const std::vector<double> w(n, 1.0 / static_cast<double>(n));
  a::ch16::Rng rng(5);
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        a::ch16::PriceBasketCall(model, w, 100, 1, 10'000, rng));
  }
  state.SetItemsProcessed(state.iterations() * 10'000);
}
BENCHMARK(BM_BasketMc)->Arg(2)->Arg(20)->Unit(benchmark::kMillisecond);

}  // namespace
