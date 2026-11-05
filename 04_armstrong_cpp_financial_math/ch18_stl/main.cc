// Chapter 18 demo: VaR from simulated P&L, a histogram, and a trade blotter.

#include <cstdio>
#include <random>
#include <vector>

#include "ch18_stl/stl.h"

namespace a = cpp4q::armstrong::ch18;

int main() {
  std::mt19937_64 rng(18);
  std::student_t_distribution<double> fat_tails(4.0);
  std::vector<double> losses(100'000);
  for (double& x : losses) x = fat_tails(rng);
  std::printf("t(4) losses: 99%% VaR %.4f, 99%% ES %.4f "
              "(t4 quantile 3.7469)\n",
              a::HistoricalVaR(losses, 0.99),
              a::HistoricalExpectedShortfall(losses, 0.99));
  std::printf("histogram (bin width 1):\n");
  for (const auto& [bin, count] : a::Histogram(losses, 1.0)) {
    if (bin < -4 || bin > 3) continue;
    std::printf("  [%3lld, %3lld): %6zu\n", bin, bin + 1, count);
  }

  const std::vector<a::Trade> trades{{"AAPL", 100, 190.0}, {"MSFT", 50, 410.0},
                                     {"AAPL", -40, 195.0}, {"TSLA", 30, 250.0},
                                     {"TSLA", -30, 240.0}, {"NVDA", 20, 900.0}};
  const auto book = a::AggregateTrades(trades);
  std::printf("\nnet positions:\n");
  for (const auto& [ticker, pos] : book) {
    std::printf("  %-5s %6.0f shares, cost %9.2f\n", ticker.c_str(),
                pos.quantity, pos.cost);
  }
  const std::map<std::string, double> marks{
      {"AAPL", 192.0}, {"MSFT", 415.0}, {"NVDA", 880.0}};
  std::printf("two largest exposures:");
  for (const auto& [ticker, value] : a::LargestExposures(book, marks, 2)) {
    std::printf(" %s %.0f", ticker.c_str(), value);
  }
  std::printf("\n");
  return 0;
}
