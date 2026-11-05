// Chapter 3: virtual dispatch vs std::variant vs a concrete type.

#include <benchmark/benchmark.h>

#include <memory>
#include <random>
#include <vector>

#include "ch03_polymorphism/payoff.h"

namespace {

namespace ch03 = cpp4q::hanson::ch03;

constexpr int kN = 4096;

std::vector<double> Spots() {
  std::mt19937_64 rng(1);
  std::uniform_real_distribution<double> u(50.0, 150.0);
  std::vector<double> s(kN);
  for (auto& x : s) x = u(rng);
  return s;
}

// A book of mixed payoffs (round-robin so the branch predictor cannot win).
void BM_VirtualPayoffs(benchmark::State& state) {
  std::vector<std::unique_ptr<ch03::Payoff>> book;
  for (int i = 0; i < kN; ++i) {
    switch (i % 3) {
      case 0:
        book.push_back(std::make_unique<ch03::CallPayoff>(100.0));
        break;
      case 1:
        book.push_back(std::make_unique<ch03::PutPayoff>(100.0));
        break;
      default:
        book.push_back(std::make_unique<ch03::DigitalCallPayoff>(100.0, 1.0));
    }
  }
  const auto spots = Spots();
  for (auto _ : state) {
    double total = 0.0;
    for (int i = 0; i < kN; ++i) total += (*book[i])(spots[i]);
    benchmark::DoNotOptimize(total);
  }
  state.SetItemsProcessed(state.iterations() * kN);
}
BENCHMARK(BM_VirtualPayoffs);

void BM_VariantPayoffs(benchmark::State& state) {
  std::vector<ch03::PayoffVariant> book;
  for (int i = 0; i < kN; ++i) {
    switch (i % 3) {
      case 0:
        book.emplace_back(ch03::Call{100.0});
        break;
      case 1:
        book.emplace_back(ch03::Put{100.0});
        break;
      default:
        book.emplace_back(ch03::DigitalCall{100.0, 1.0});
    }
  }
  const auto spots = Spots();
  for (auto _ : state) {
    double total = 0.0;
    for (int i = 0; i < kN; ++i) total += ch03::Evaluate(book[i], spots[i]);
    benchmark::DoNotOptimize(total);
  }
  state.SetItemsProcessed(state.iterations() * kN);
}
BENCHMARK(BM_VariantPayoffs);

void BM_ConcreteCalls(benchmark::State& state) {
  const ch03::Call call{100.0};
  const auto spots = Spots();
  for (auto _ : state) {
    double total = 0.0;
    for (int i = 0; i < kN; ++i) total += call(spots[i]);
    benchmark::DoNotOptimize(total);
  }
  state.SetItemsProcessed(state.iterations() * kN);
}
BENCHMARK(BM_ConcreteCalls);

}  // namespace
