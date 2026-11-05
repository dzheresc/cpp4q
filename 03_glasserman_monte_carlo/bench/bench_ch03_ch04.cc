// Chapters 3-4: path generation and the variance-reduction estimators.
// Each estimator reports its standard error, so efficiency (variance x
// time) can be compared: see profiling/REPORT.md.

#include <benchmark/benchmark.h>

#include <vector>

#include "ch02_random_numbers/normal.h"
#include "ch03_sample_paths/brownian.h"
#include "ch03_sample_paths/gbm.h"
#include "ch03_sample_paths/jump_diffusion.h"
#include "ch03_sample_paths/short_rate.h"
#include "ch04_variance_reduction/variance_reduction.h"

namespace {

namespace g = cpp4q::glasserman;
using g::ch02::Mrg32k3a;
using g::ch02::Sampler;

void BM_RandomWalk(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const auto t = g::ch03::UniformGrid(1.0, n);
  std::vector<double> z(n, 0.3), w(n);
  for (auto _ : state) {
    g::ch03::RandomWalkPath(t, z, w);
    benchmark::DoNotOptimize(w.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_RandomWalk)->Arg(16)->Arg(256);

void BM_BrownianBridge(benchmark::State& state) {
  const auto n = static_cast<std::size_t>(state.range(0));
  const g::ch03::BrownianBridge bb(g::ch03::UniformGrid(1.0, n));
  std::vector<double> z(n, 0.3), w(n);
  for (auto _ : state) {
    bb.Build(z, w);
    benchmark::DoNotOptimize(w.data());
  }
  state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_BrownianBridge)->Arg(16)->Arg(256);

void BM_GbmPath(benchmark::State& state) {
  const auto t = g::ch03::UniformGrid(1.0, 64);
  Sampler s(Mrg32k3a(1));
  std::vector<double> z(64), out(64);
  for (auto _ : state) {
    s.FillNormal(z);
    g::ch03::GbmPath({}, t, z, out);
    benchmark::DoNotOptimize(out.data());
  }
  state.SetItemsProcessed(state.iterations() * 64);
}
BENCHMARK(BM_GbmPath);

void BM_MultiAssetTerminal(benchmark::State& state) {
  const auto d = static_cast<std::size_t>(state.range(0));
  std::vector<double> corr(d * d, 0.3);
  for (std::size_t i = 0; i < d; ++i) corr[i * d + i] = 1.0;
  const g::ch03::MultiAssetGbm m(std::vector<g::ch03::GbmParams>(d), corr);
  Sampler s(Mrg32k3a(2));
  std::vector<double> z(d), out(d);
  for (auto _ : state) {
    s.FillNormal(z);
    m.Terminal(1.0, z, out);
    benchmark::DoNotOptimize(out.data());
  }
}
BENCHMARK(BM_MultiAssetTerminal)->Arg(10)->Arg(50);

void BM_VasicekBond(benchmark::State& state) {
  Sampler s(Mrg32k3a(3));
  const g::ch03::ShortRateParams p{.kappa = 0.3, .theta = 0.05, .sigma = 0.02};
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch03::VasicekBondMc(p, 0.03, 5, 20, 10'000, s));
  }
  state.SetItemsProcessed(state.iterations() * 200'000);
}
BENCHMARK(BM_VasicekBond)->Unit(benchmark::kMillisecond);

void BM_CirBondExact(benchmark::State& state) {
  Sampler s(Mrg32k3a(4));
  const g::ch03::ShortRateParams p{.kappa = 0.3, .theta = 0.05, .sigma = 0.1};
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch03::CirBondMc(p, 0.03, 5, 20, 10'000, s));
  }
  state.SetItemsProcessed(state.iterations() * 200'000);
}
BENCHMARK(BM_CirBondExact)->Unit(benchmark::kMillisecond);

void BM_MertonCall(benchmark::State& state) {
  Sampler s(Mrg32k3a(5));
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch03::MertonCallMc({}, 100, 1, 100'000, s));
  }
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_MertonCall)->Unit(benchmark::kMillisecond);

// ---- Chapter 4: equal sample budgets; SE reported per estimator.

constexpr std::size_t kPaths = 200'000;

template <typename F>
void RunEstimator(benchmark::State& state, F f) {
  Sampler s(Mrg32k3a(6));
  g::ch01::Estimate e{};
  for (auto _ : state) {
    e = f(s);
    benchmark::DoNotOptimize(e);
  }
  state.counters["std_error"] = e.std_error;
  state.SetItemsProcessed(state.iterations() * static_cast<long>(kPaths));
}

const g::ch01::BlackScholesInputs kAtm{};
const g::ch04::AsianSpec kAsian{kAtm, 12};

void BM_VrPlainCall(benchmark::State& st) {
  RunEstimator(st,
               [](Sampler& s) { return g::ch04::PlainCall(kAtm, kPaths, s); });
}
void BM_VrAntitheticCall(benchmark::State& st) {
  RunEstimator(st, [](Sampler& s) {
    return g::ch04::AntitheticCall(kAtm, kPaths / 2, s);
  });
}
void BM_VrControlCall(benchmark::State& st) {
  RunEstimator(st, [](Sampler& s) {
    return g::ch04::ControlVariateCall(kAtm, kPaths, s).estimate;
  });
}
void BM_VrStratifiedCall(benchmark::State& st) {
  RunEstimator(st, [](Sampler& s) {
    return g::ch04::StratifiedCall(kAtm, 200, kPaths / 200, s);
  });
}
void BM_VrImportanceOtmCall(benchmark::State& st) {
  g::ch01::BlackScholesInputs otm = kAtm;
  otm.strike = 160;
  const double shift = g::ch04::StrikeCenteringShift(otm);
  RunEstimator(st, [&](Sampler& s) {
    return g::ch04::ImportanceSamplingCall(otm, shift, kPaths, s);
  });
}
void BM_VrPlainAsian(benchmark::State& st) {
  RunEstimator(
      st, [](Sampler& s) { return g::ch04::PlainAsian(kAsian, kPaths, s); });
}
void BM_VrGeometricControlAsian(benchmark::State& st) {
  RunEstimator(st, [](Sampler& s) {
    return g::ch04::GeometricControlAsian(kAsian, kPaths, s).estimate;
  });
}
void BM_VrTerminalStratifiedAsian(benchmark::State& st) {
  RunEstimator(st, [](Sampler& s) {
    return g::ch04::TerminalStratifiedAsian(kAsian, 200, kPaths / 200, s);
  });
}
BENCHMARK(BM_VrPlainCall)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrAntitheticCall)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrControlCall)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrStratifiedCall)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrImportanceOtmCall)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrPlainAsian)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrGeometricControlAsian)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_VrTerminalStratifiedAsian)->Unit(benchmark::kMillisecond);

}  // namespace
