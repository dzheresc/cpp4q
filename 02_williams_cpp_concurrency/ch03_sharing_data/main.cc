// Chapter 3 demo: deadlock-free transfers, lock hierarchy, reader/writer
// quote cache, lazy curve.

#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ch03_sharing_data/account.h"
#include "ch03_sharing_data/hierarchical_mutex.h"
#include "ch03_sharing_data/quote_cache.h"

int main() {
  namespace ch03 = cpp4q::williams::ch03;

  ch03::Account a(1'000'000), b(1'000'000);
  {
    std::jthread ab([&] {
      for (int i = 0; i < 100'000; ++i) ch03::Transfer(a, b, 1);
    });
    std::jthread ba([&] {
      for (int i = 0; i < 100'000; ++i) ch03::Transfer(b, a, 1);
    });
  }
  std::printf("after 200k crossing transfers: a=%lld b=%lld (total %lld)\n",
              static_cast<long long>(a.Balance()),
              static_cast<long long>(b.Balance()),
              static_cast<long long>(a.Balance() + b.Balance()));

  ch03::HierarchicalMutex high(10'000), low(5'000);
  {
    std::scoped_lock ok_order(high);
    std::scoped_lock inner(low);
  }
  try {
    std::scoped_lock wrong(low);
    std::scoped_lock boom(high);
  } catch (const std::logic_error& e) {
    std::printf("caught: %s\n", e.what());
  }

  ch03::QuoteCache cache;
  cache.Update("EURUSD", {1.0841, 1.0843});
  if (auto q = cache.Get("EURUSD")) std::printf("EURUSD mid %.5f\n", q->Mid());

  int builds = 0;
  ch03::LazyCurve curve([&] {
    ++builds;
    return ch03::ZeroCurve{{0.5, 1, 2, 5, 10},
                           {0.04, 0.041, 0.042, 0.04, 0.039}};
  });
  {
    std::vector<std::jthread> readers;
    for (int i = 0; i < 4; ++i) {
      readers.emplace_back([&] { (void)curve.Get().Discount(3.0); });
    }
  }
  std::printf("DF(3y) = %.6f, curve built %d time(s)\n",
              curve.Get().Discount(3.0), builds);
}
