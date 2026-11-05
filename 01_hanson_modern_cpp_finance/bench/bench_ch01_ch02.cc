// Chapters 1-2: closed-form pricing and copy vs move.

#include <benchmark/benchmark.h>

#include <utility>
#include <vector>

#include "ch01_overview/black_scholes.h"
#include "ch02_classes/price_buffer.h"

namespace {

using cpp4q::hanson::ch01::BlackScholesInputs;
using cpp4q::hanson::ch01::OptionType;
using cpp4q::hanson::ch02::PriceBuffer;

void BM_BlackScholesPrice(benchmark::State& state) {
  BlackScholesInputs in{100.0, 100.0, 0.03, 0.01, 0.2, 1.0};
  for (auto _ : state) {
    benchmark::DoNotOptimize(in);
    benchmark::DoNotOptimize(
        cpp4q::hanson::ch01::BlackScholesPrice(OptionType::kCall, in));
  }
}
BENCHMARK(BM_BlackScholesPrice);

void BM_BlackScholesGreeks(benchmark::State& state) {
  BlackScholesInputs in{100.0, 100.0, 0.03, 0.01, 0.2, 1.0};
  for (auto _ : state) {
    benchmark::DoNotOptimize(in);
    benchmark::DoNotOptimize(
        cpp4q::hanson::ch01::BlackScholesGreeks(OptionType::kPut, in));
  }
}
BENCHMARK(BM_BlackScholesGreeks);

void BM_PriceBufferCopy(benchmark::State& state) {
  const PriceBuffer src(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    PriceBuffer copy = src;
    benchmark::DoNotOptimize(copy);
  }
  state.SetBytesProcessed(state.iterations() * state.range(0) * 8);
}
BENCHMARK(BM_PriceBufferCopy)->Range(64, 1 << 16);

void BM_PriceBufferMove(benchmark::State& state) {
  PriceBuffer a(static_cast<std::size_t>(state.range(0)));
  for (auto _ : state) {
    PriceBuffer b = std::move(a);
    a = std::move(b);
    benchmark::DoNotOptimize(a);
  }
}
BENCHMARK(BM_PriceBufferMove)->Range(64, 1 << 16);

}  // namespace
