// Chapter 4 demo: producer/consumer queue, futures, async sort, barrier.

#include <cstdio>
#include <random>
#include <thread>
#include <vector>

#include "ch04_synchronizing/futures.h"
#include "ch04_synchronizing/thread_safe_queue.h"

int main() {
  namespace ch04 = cpp4q::williams::ch04;

  struct Tick {
    int seq;
    double price;
  };
  ch04::ThreadSafeQueue<Tick> ticks;
  double vwap_num = 0.0;
  int count = 0;
  {
    std::jthread consumer([&] {
      while (auto t = ticks.WaitAndPop()) {
        vwap_num += t->price;
        ++count;
      }
    });
    for (int i = 0; i < 1000; ++i) ticks.Push({i, 100.0 + 0.01 * (i % 7)});
    ticks.Close();
  }
  std::printf("consumed %d ticks, mean %.4f\n", count, vwap_num / count);

  ch04::PricingService service;
  auto f1 = service.Submit([] { return 42.0; });
  auto f2 = service.Submit([]() -> double { throw std::runtime_error("bad"); });
  std::printf("job 1 -> %.1f\n", f1.get());
  try {
    (void)f2.get();
  } catch (const std::exception& e) {
    std::printf("job 2 threw: %s\n", e.what());
  }

  std::vector<double> pnl(10'000);
  std::mt19937_64 rng(1);
  std::normal_distribution<double> n(0.0, 1e4);
  for (auto& x : pnl) x = n(rng);
  std::printf("99%% VaR = %.0f\n", ch04::HistoricalVarAsync(pnl, 0.99).get());

  ch04::ParallelQuickSort(pnl);
  std::printf("sorted: min %.0f max %.0f\n", pnl.front(), pnl.back());

  const auto means =
      ch04::LockstepMeanSpot(20'000, 12, 100.0, 0.2, 1.0 / 12, 4, 42);
  std::printf("mean spot after 12 monthly steps: %.3f (martingale ~100)\n",
              means.back());
}
