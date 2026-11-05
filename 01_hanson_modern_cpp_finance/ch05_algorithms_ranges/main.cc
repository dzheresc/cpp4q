// Chapter 5 demo: statistics on a synthetic price series.

#include <cmath>
#include <format>
#include <iostream>
#include <ranges>
#include <vector>

#include "ch05_algorithms_ranges/return_stats.h"

int main() {
  namespace ch05 = cpp4q::hanson::ch05;

  // A deterministic zig-zag price path built with a view pipeline.
  auto path = std::views::iota(0, 250) | std::views::transform([](int i) {
                return 100.0 * std::exp(0.0004 * i + 0.02 * std::sin(0.3 * i));
              });
  const std::vector<double> prices(path.begin(), path.end());
  const auto returns = ch05::SimpleReturns(prices);

  std::cout << std::format("mean {:.5f}  vol {:.5f}  sharpe {:.3f}\n",
                           ch05::Mean(returns), ch05::SampleStdDev(returns),
                           ch05::SharpeRatio(returns, 0.0, 252.0))
            << std::format("max drawdown {:.2f}%  99% VaR {:.2f}%\n",
                           100.0 * ch05::MaxDrawdown(prices),
                           100.0 * ch05::HistoricalVar(returns, 0.99));

  const std::vector<ch05::Ticker> universe{
      {"AAA", 0.12}, {"BBB", -0.03}, {"CCC", 0.31}, {"DDD", 0.07}};
  for (const auto& t : ch05::TopPerformers(universe, 2)) {
    std::cout << t.symbol << ' ' << t.total_return << '\n';
  }
}
