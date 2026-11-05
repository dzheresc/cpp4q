// Chapters 1-2: uniform and normal generation, non-uniform variates,
// substream jumps, and parallel scaling.

#include <benchmark/benchmark.h>

#include <cmath>
#include <random>

#include "ch01_foundations/monte_carlo.h"
#include "ch02_random_numbers/distributions.h"
#include "ch02_random_numbers/normal.h"
#include "ch02_random_numbers/parallel_mc.h"
#include "ch02_random_numbers/rng.h"

namespace {

namespace g = cpp4q::glasserman;
using g::ch02::Mrg32k3a;
using g::ch02::NormalMethod;
using g::ch02::Sampler;

constexpr int kBatch = 1 << 16;

void BM_Mt19937Uniform(benchmark::State& state) {
  std::mt19937_64 rng(1);
  std::uniform_real_distribution<double> u;
  for (auto _ : state) {
    double sum = 0;
    for (int i = 0; i < kBatch; ++i) sum += u(rng);
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_Mt19937Uniform);

void BM_Mrg32k3aUniform(benchmark::State& state) {
  Mrg32k3a rng(1);
  for (auto _ : state) {
    double sum = 0;
    for (int i = 0; i < kBatch; ++i) sum += rng.NextUniform();
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_Mrg32k3aUniform);

void BM_StdNormalDistribution(benchmark::State& state) {
  std::mt19937_64 rng(1);
  std::normal_distribution<double> z;
  for (auto _ : state) {
    double sum = 0;
    for (int i = 0; i < kBatch; ++i) sum += z(rng);
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_StdNormalDistribution);

void BM_SamplerNormal(benchmark::State& state) {
  Sampler s(Mrg32k3a(1), static_cast<NormalMethod>(state.range(0)));
  for (auto _ : state) {
    double sum = 0;
    for (int i = 0; i < kBatch; ++i) sum += s.Normal();
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
  state.SetLabel(state.range(0) == 0   ? "inversion"
                 : state.range(0) == 1 ? "box-muller"
                                       : "polar");
}
BENCHMARK(BM_SamplerNormal)->DenseRange(0, 2);

void BM_SamplerFillNormal(benchmark::State& state) {
  Sampler s(Mrg32k3a(1));
  std::vector<double> z(kBatch);
  for (auto _ : state) {
    s.FillNormal(z);
    benchmark::DoNotOptimize(z.data());
  }
  state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_SamplerFillNormal);

void BM_InverseNormalCdf(benchmark::State& state) {
  const bool accurate = state.range(0) != 0;
  for (auto _ : state) {
    double sum = 0;
    for (int i = 1; i < kBatch; ++i) {
      const double u = i / static_cast<double>(kBatch);
      sum += accurate ? g::ch02::InverseNormalCdfAccurate(u)
                      : g::ch02::InverseNormalCdf(u);
    }
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * (kBatch - 1));
  state.SetLabel(accurate ? "BSM+Halley" : "BSM");
}
BENCHMARK(BM_InverseNormalCdf)->Arg(0)->Arg(1);

void BM_Gamma(benchmark::State& state) {
  Sampler s(Mrg32k3a(2));
  const double shape = static_cast<double>(state.range(0)) / 10.0;
  for (auto _ : state) {
    double sum = 0;
    for (int i = 0; i < 4096; ++i) sum += g::ch02::SampleGamma(s, shape);
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_Gamma)->Arg(5)->Arg(25)->Arg(300);

void BM_Poisson(benchmark::State& state) {
  Sampler s(Mrg32k3a(3));
  const auto mean = static_cast<double>(state.range(0));
  for (auto _ : state) {
    std::uint64_t sum = 0;
    for (int i = 0; i < 4096; ++i) sum += g::ch02::SamplePoisson(s, mean);
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_Poisson)->Arg(1)->Arg(20)->Arg(500);

void BM_NoncentralChiSquare(benchmark::State& state) {
  Sampler s(Mrg32k3a(4));
  for (auto _ : state) {
    double sum = 0;
    for (int i = 0; i < 4096; ++i) {
      sum += g::ch02::SampleNoncentralChiSquare(s, 1.6, 30.0);
    }
    benchmark::DoNotOptimize(sum);
  }
  state.SetItemsProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_NoncentralChiSquare);

void BM_SubstreamJump(benchmark::State& state) {
  const Mrg32k3a base(5);
  std::uint64_t k = 0;
  for (auto _ : state) benchmark::DoNotOptimize(base.Substream(++k));
}
BENCHMARK(BM_SubstreamJump);

void BM_Ch01EuropeanCall(benchmark::State& state) {
  const g::ch01::BlackScholesInputs in{};
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch01::PriceEuropeanCall(in, 100'000, 1));
  }
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_Ch01EuropeanCall)->Unit(benchmark::kMillisecond);

void BM_ParallelCall(benchmark::State& state) {
  const auto threads = static_cast<unsigned>(state.range(0));
  auto path = [](Sampler& s) {
    const double st = 100.0 * std::exp(0.03 + 0.2 * s.Normal());
    return std::exp(-0.05) * std::max(st - 100.0, 0.0);
  };
  constexpr std::size_t kPaths = 2'000'000;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        g::ch02::ParallelSimulate(path, kPaths, {.threads = threads}));
  }
  state.SetItemsProcessed(state.iterations() * kPaths);
}
BENCHMARK(BM_ParallelCall)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

}  // namespace
