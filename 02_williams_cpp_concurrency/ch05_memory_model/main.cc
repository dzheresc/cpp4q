// Chapter 5 demo: spin lock, seqlock top-of-book, relaxed counters.

#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#include "ch05_memory_model/atomic_stats.h"
#include "ch05_memory_model/seq_lock.h"
#include "ch05_memory_model/spin_lock.h"

int main() {
  namespace ch05 = cpp4q::williams::ch05;

  ch05::SpinLock lock;
  long long notional = 0;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&] {
        for (int i = 0; i < 100'000; ++i) {
          std::lock_guard g(lock);
          notional += 10;
        }
      });
    }
  }
  std::printf("spin-locked notional: %lld (expect 4000000)\n", notional);

  ch05::SeqLock<ch05::TopOfBook> book;
  long long torn = 0;
  {
    std::jthread writer([&] {
      for (std::int64_t i = 1; i <= 200'000; ++i) {
        const double px = 100.0 + 0.01 * static_cast<double>(i % 100);
        book.Store({px, px + 0.01, i, i});
      }
    });
    std::jthread reader([&] {
      for (int i = 0; i < 200'000; ++i) {
        const auto b = book.Load();
        if (b.bid_size != b.ask_size) ++torn;
      }
    });
  }
  std::printf("seqlock: version %llu, torn reads %lld\n",
              static_cast<unsigned long long>(book.version()), torn);

  ch05::TradeCounters counters;
  {
    std::jthread a([&] {
      for (int i = 0; i < 1000; ++i) counters.RecordFill(100);
    });
    std::jthread b([&] {
      for (int i = 0; i < 1000; ++i) counters.RecordReject();
    });
  }
  std::printf("fills %lld volume %lld rejects %lld\n",
              static_cast<long long>(counters.fills()),
              static_cast<long long>(counters.volume()),
              static_cast<long long>(counters.rejects()));
}
