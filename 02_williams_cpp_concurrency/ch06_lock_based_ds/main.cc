// Chapter 6 demo: two-lock queue, bucketed position book, hand-over-hand
// list of resting orders.

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "ch06_lock_based_ds/fine_grained_queue.h"
#include "ch06_lock_based_ds/lookup_table.h"
#include "ch06_lock_based_ds/thread_safe_list.h"

int main() {
  namespace ch06 = cpp4q::williams::ch06;

  ch06::FineGrainedQueue<int> q;
  long long sum = 0;
  {
    std::jthread consumer([&] {
      for (int i = 0; i < 100'000; ++i) sum += q.WaitAndPop();
    });
    for (int i = 0; i < 100'000; ++i) q.Push(i);
  }
  std::printf("queue sum %lld (expect %lld)\n", sum, 99'999LL * 100'000 / 2);

  ch06::ThreadSafeLookupTable<std::string, long long> positions;
  const std::vector<std::string> symbols = {"AAPL", "MSFT", "NVDA", "SPY"};
  {
    std::vector<std::jthread> traders;
    for (int t = 0; t < 4; ++t) {
      traders.emplace_back([&, t] {
        for (int i = 0; i < 10'000; ++i) {
          positions.Modify(symbols[static_cast<std::size_t>(i % 4)],
                           [&](long long& qty) { qty += (t % 2 ? 1 : 2); });
        }
      });
    }
  }
  for (const auto& [sym, qty] : positions.Snapshot()) {
    std::printf("  %-5s %lld\n", sym.c_str(), qty);
  }

  struct Order {
    int id;
    double price;
  };
  ch06::ThreadSafeList<Order> resting;
  for (int i = 0; i < 10; ++i) resting.PushFront({i, 100.0 + i});
  const auto removed =
      resting.RemoveIf([](const Order& o) { return o.price > 105.0; });
  double total = 0;
  resting.ForEach([&](const Order& o) { total += o.price; });
  std::printf("cancelled %zu orders above 105, remaining price sum %.1f\n",
              removed, total);
}
