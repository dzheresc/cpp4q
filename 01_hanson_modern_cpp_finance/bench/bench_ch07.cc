// Chapter 7: schedules, bond pricing and yield solving.

#include <benchmark/benchmark.h>

#include <chrono>

#include "ch07_dates_fixed_income/bond.h"

namespace {

namespace ch07 = cpp4q::hanson::ch07;
using namespace std::chrono;  // NOLINT(build/namespaces) date literals

const ch07::ScheduleSpec kSpec{.effective = 2020y / January / 15d,
                               .maturity = 2050y / January / 15d,
                               .frequency = ch07::Frequency::kSemiAnnual};

void BM_GenerateSchedule(benchmark::State& state) {
  const ch07::Calendar cal;
  for (auto _ : state) {
    benchmark::DoNotOptimize(ch07::GenerateSchedule(kSpec, cal));
  }
}
BENCHMARK(BM_GenerateSchedule);

void BM_YearFraction(benchmark::State& state) {
  const auto dc = static_cast<ch07::DayCount>(state.range(0));
  ch07::Date a = 2024y / January / 31d;
  ch07::Date b = 2031y / August / 15d;
  for (auto _ : state) {
    benchmark::DoNotOptimize(a);
    benchmark::DoNotOptimize(ch07::YearFraction(a, b, dc));
  }
}
BENCHMARK(BM_YearFraction)->DenseRange(0, 4);

void BM_BondRisk(benchmark::State& state) {
  const ch07::FixedRateBond bond(100.0, 0.04, kSpec, ch07::DayCount::kThirty360,
                                 ch07::Calendar{});
  const ch07::Date settle = 2025y / March / 3d;
  for (auto _ : state) benchmark::DoNotOptimize(bond.Risk(settle, 0.045));
}
BENCHMARK(BM_BondRisk);

void BM_BondYield(benchmark::State& state) {
  const ch07::FixedRateBond bond(100.0, 0.04, kSpec, ch07::DayCount::kThirty360,
                                 ch07::Calendar{});
  const ch07::Date settle = 2025y / March / 3d;
  for (auto _ : state) {
    benchmark::DoNotOptimize(bond.YieldFromCleanPrice(settle, 93.25));
  }
}
BENCHMARK(BM_BondYield);

}  // namespace
