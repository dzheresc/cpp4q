// Chapter 8 demo: parallel for_each / find / scan, false sharing, VaR.

#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

#include "ch08_designing_concurrent_code/parallel_algorithms.h"

int main() {
  namespace ch08 = cpp4q::williams::ch08;

  std::vector<double> pnl(2'000'000);
  std::mt19937_64 rng(5);
  std::normal_distribution<double> n(0.0, 1000.0);
  for (auto& x : pnl) x = n(rng);

  const auto breach = ch08::ParallelFind(pnl.begin(), pnl.end(),
                                         [](double x) { return x < -4500.0; });
  std::printf("first 4.5-sigma loss at index %td\n", breach - pnl.begin());

  std::vector<double> cumulative(pnl.size());
  ch08::ParallelInclusiveScan<double>(pnl, cumulative);
  std::printf("cumulative PnL: %.1f\n", cumulative.back());

  std::vector<std::int32_t> ids(4'000'000);
  for (std::size_t i = 0; i < ids.size(); ++i) {
    ids[i] = static_cast<std::int32_t>(i);
  }
  for (auto* f : {&ch08::CountByBucketPacked, &ch08::CountByBucketPadded}) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto counts = f(ids, 4);
    const auto ms = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - t0)
                        .count();
    std::printf("%s: bucket0=%lld in %.2f ms\n",
                f == &ch08::CountByBucketPacked ? "packed" : "padded",
                static_cast<long long>(counts[0]), ms);
  }

  constexpr std::size_t kFactors = 20, kScenarios = 10'000;
  std::vector<double> exposures(kFactors, 1e5);
  std::vector<double> scen(kFactors * kScenarios);
  std::normal_distribution<double> shock(0.0, 0.01);
  for (auto& s : scen) s = shock(rng);
  std::printf("99%% scenario VaR: %.0f\n",
              ch08::ScenarioVar(exposures, scen, 0.99));
}
