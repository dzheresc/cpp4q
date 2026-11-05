// Chapters 7-9: Greeks, American options, and risk.

#include <benchmark/benchmark.h>

#include <vector>

#include "ch07_sensitivities/greeks.h"
#include "ch08_american_options/lattice.h"
#include "ch08_american_options/lsm.h"
#include "ch08_american_options/random_tree.h"
#include "ch09_risk_management/credit.h"
#include "ch09_risk_management/portfolio_var.h"

namespace {

namespace g = cpp4q::glasserman;
using g::ch01::OptionType;

void BM_CallGreeks(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch07::EuropeanCallGreeks({}, 100'000, 1));
  }
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_CallGreeks)->Unit(benchmark::kMillisecond);

void BM_FiniteDifferenceDelta(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch07::FiniteDifferenceDelta(
        {}, 0.01, g::ch07::FiniteDifference::kCentral, true, 100'000, 1));
  }
  state.SetItemsProcessed(state.iterations() * 100'000);
}
BENCHMARK(BM_FiniteDifferenceDelta)->Unit(benchmark::kMillisecond);

void BM_AsianDelta(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch07::AsianCallDelta({}, 12, 50'000, 1));
  }
  state.SetItemsProcessed(state.iterations() * 50'000);
}
BENCHMARK(BM_AsianDelta)->Unit(benchmark::kMillisecond);

const g::ch01::BlackScholesInputs kPutMarket{
    .spot = 100, .strike = 110, .rate = 0.06, .vol = 0.25, .maturity = 1};

void BM_CrrAmerican(benchmark::State& state) {
  const auto steps = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        g::ch08::CrrPrice(kPutMarket, OptionType::kPut, steps,
                          g::ch08::ExerciseStyle::kAmerican));
  }
}
BENCHMARK(BM_CrrAmerican)->Arg(1000)->Arg(4000)->Unit(benchmark::kMillisecond);

void BM_LsmTrain(benchmark::State& state) {
  const auto dates = static_cast<std::size_t>(state.range(0));
  const g::ch08::BermudanSpec spec{kPutMarket, OptionType::kPut, dates};
  for (auto _ : state) {
    g::ch08::LsmPricer lsm(spec, 4);
    benchmark::DoNotOptimize(lsm.Train(50'000, 1));
  }
  state.SetItemsProcessed(state.iterations() * 50'000 * state.range(0));
}
BENCHMARK(BM_LsmTrain)->Arg(10)->Arg(50)->Unit(benchmark::kMillisecond);

void BM_LsmPrice(benchmark::State& state) {
  const g::ch08::BermudanSpec spec{kPutMarket, OptionType::kPut, 50};
  g::ch08::LsmPricer lsm(spec, 4);
  lsm.Train(50'000, 1);
  for (auto _ : state) benchmark::DoNotOptimize(lsm.Price(50'000, 2));
  state.SetItemsProcessed(state.iterations() * 50'000 * 50);
}
BENCHMARK(BM_LsmPrice)->Unit(benchmark::kMillisecond);

void BM_RandomTree(benchmark::State& state) {
  const g::ch08::BermudanSpec spec{kPutMarket, OptionType::kPut, 3};
  const auto b = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    benchmark::DoNotOptimize(g::ch08::RandomTreePrice(spec, b, 10, 1));
  }
  state.SetItemsProcessed(state.iterations() * 10 * state.range(0) *
                          state.range(0) * state.range(0));
}
BENCHMARK(BM_RandomTree)->Arg(20)->Arg(50)->Unit(benchmark::kMillisecond);

g::ch09::OptionPortfolio Book() {
  g::ch09::Market m{
      .spots = {100.0, 50.0, 200.0},
      .vols = {0.3, 0.25, 0.2},
      .correlation = {1.0, 0.5, 0.3, 0.5, 1.0, 0.4, 0.3, 0.4, 1.0},
      .rate = 0.04};
  return g::ch09::OptionPortfolio(m,
                                  {{0, OptionType::kCall, 100.0, 0.5, -10.0},
                                   {0, OptionType::kPut, 95.0, 0.5, -10.0},
                                   {1, OptionType::kCall, 55.0, 0.25, -20.0},
                                   {2, OptionType::kPut, 190.0, 1.0, 5.0},
                                   {2, OptionType::kCall, 210.0, 1.0, -15.0}});
}

void BM_SimulateLosses(benchmark::State& state) {
  const auto book = Book();
  const bool dg = state.range(0) != 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        g::ch09::SimulateLosses(book, 10.0 / 252, 50'000, dg, 1));
  }
  state.SetItemsProcessed(state.iterations() * 50'000);
  state.SetLabel(dg ? "delta-gamma" : "full revaluation");
}
BENCHMARK(BM_SimulateLosses)->Arg(0)->Arg(1)->Unit(benchmark::kMillisecond);

void BM_TailIsDeltaGamma(benchmark::State& state) {
  const auto book = Book();
  g::ch01::Estimate e{};
  for (auto _ : state) {
    e = g::ch09::TailProbabilityIs(book, 10.0 / 252, 330.0, 20'000, 2);
    benchmark::DoNotOptimize(e);
  }
  state.counters["std_error"] = e.std_error;
  state.SetItemsProcessed(state.iterations() * 20'000);
}
BENCHMARK(BM_TailIsDeltaGamma)->Unit(benchmark::kMillisecond);

g::ch09::GaussianCopulaPortfolio CreditBook() {
  std::vector<g::ch09::Obligor> obligors;
  for (int k = 0; k < 100; ++k) {
    obligors.push_back(
        {0.01 * (1 + k % 5), 1.0 + (k % 3), 0.3 + 0.05 * (k % 4)});
  }
  return g::ch09::GaussianCopulaPortfolio(obligors);
}

void BM_CreditTailPlain(benchmark::State& state) {
  const auto book = CreditBook();
  for (auto _ : state) {
    benchmark::DoNotOptimize(book.TailProbability(40.0, 10'000, 3));
  }
  state.SetItemsProcessed(state.iterations() * 10'000);
}
BENCHMARK(BM_CreditTailPlain)->Unit(benchmark::kMillisecond);

void BM_CreditTailIs(benchmark::State& state) {
  const auto book = CreditBook();
  const double shift = book.FindFactorShift(40.0);
  for (auto _ : state) {
    benchmark::DoNotOptimize(book.TailProbabilityIs(40.0, shift, 10'000, 3));
  }
  state.SetItemsProcessed(state.iterations() * 10'000);
}
BENCHMARK(BM_CreditTailIs)->Unit(benchmark::kMillisecond);

void BM_CreditFindShift(benchmark::State& state) {
  const auto book = CreditBook();
  for (auto _ : state) benchmark::DoNotOptimize(book.FindFactorShift(40.0));
}
BENCHMARK(BM_CreditFindShift)->Unit(benchmark::kMillisecond);

}  // namespace
