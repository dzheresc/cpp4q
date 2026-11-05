// Chapter 9 demo: work-stealing pool, recursive sort on the pool,
// cancellable background repricer.

#include <cstdio>
#include <numeric>
#include <random>
#include <vector>

#include "ch09_advanced_thread_mgmt/thread_pool.h"

int main() {
  namespace ch09 = cpp4q::williams::ch09;

  ch09::ThreadPool pool(4);
  std::vector<std::future<double>> legs;
  for (int i = 1; i <= 8; ++i) {
    legs.push_back(pool.Submit([i] { return 1e6 * 0.01 * i; }));
  }
  double total = 0.0;
  for (auto& f : legs) total += f.get();
  std::printf("pool priced 8 legs: total %.0f\n", total);

  std::vector<double> prices(1'000'000);
  std::mt19937_64 rng(9);
  std::uniform_real_distribution<double> u(90.0, 110.0);
  for (auto& p : prices) p = u(rng);
  ch09::PoolQuickSort(pool, prices);
  std::printf("pool sort: sorted=%d\n",
              std::is_sorted(prices.begin(), prices.end()));

  ch09::BackgroundRepricer repricer(1'000.0);
  for (int i = 1; i <= 5; ++i) repricer.OnSpot(100.0 + i);
  std::printf("latest value: %.0f\n", repricer.WaitForValue(5));
  repricer.Stop();
}
