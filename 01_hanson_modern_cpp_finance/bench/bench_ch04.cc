// Chapter 4: container-heavy workloads.

#include <benchmark/benchmark.h>

#include <random>
#include <string>
#include <vector>

#include "ch04_stl_containers/order_book.h"
#include "ch04_stl_containers/portfolio.h"
#include "ch04_stl_containers/rolling_window.h"

namespace {

namespace ch04 = cpp4q::hanson::ch04;

void BM_OrderBookUpdates(benchmark::State& state) {
  std::mt19937_64 rng(2);
  std::uniform_int_distribution<int> tick(-50, 50);
  std::uniform_int_distribution<int> qty(-100, 200);
  ch04::OrderBook book;
  for (auto _ : state) {
    const int t = tick(rng);
    const auto side = t < 0 ? ch04::Side::kBid : ch04::Side::kAsk;
    book.Update(side, 100.0 + 0.01 * t, qty(rng));
    benchmark::DoNotOptimize(book.BestBid());
  }
}
BENCHMARK(BM_OrderBookUpdates);

void BM_PortfolioMarketValue(benchmark::State& state) {
  const auto n = static_cast<int>(state.range(0));
  ch04::Portfolio pf;
  ch04::PriceMap prices;
  for (int i = 0; i < n; ++i) {
    const std::string t = "TICKER" + std::to_string(i);
    pf.Trade(t, 100.0 + i, 10.0);
    prices.emplace(t, 11.0);
  }
  for (auto _ : state) benchmark::DoNotOptimize(pf.MarketValue(prices));
  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_PortfolioMarketValue)->Range(16, 4096);

void BM_RollingWindowPush(benchmark::State& state) {
  ch04::RollingWindow w(static_cast<std::size_t>(state.range(0)));
  double x = 100.0;
  for (auto _ : state) {
    x += 0.001;
    w.Push(x);
    benchmark::DoNotOptimize(w.Variance());
  }
}
BENCHMARK(BM_RollingWindowPush)->Arg(20)->Arg(250);

}  // namespace
