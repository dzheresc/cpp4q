// Chapter 7 demo: SPSC market-data ring, MPMC order queue, Treiber stack.

#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

#include "ch07_lock_free/lock_free_stack.h"
#include "ch07_lock_free/mpmc_queue.h"
#include "ch07_lock_free/spsc_ring_buffer.h"

int main() {
  namespace ch07 = cpp4q::williams::ch07;

  struct Tick {
    std::uint64_t seq = 0;
    double price = 0.0;
  };
  ch07::SpscRingBuffer<Tick, 1024> feed;
  constexpr std::uint64_t kTicks = 1'000'000;
  std::uint64_t out_of_order = 0;
  {
    std::jthread strategy([&] {
      std::uint64_t expected = 0;
      while (expected < kTicks) {
        if (auto t = feed.TryPop()) {
          if (t->seq != expected) ++out_of_order;
          ++expected;
        } else {
          std::this_thread::yield();
        }
      }
    });
    for (std::uint64_t i = 0; i < kTicks;) {
      if (feed.TryPush({i, 100.0})) {
        ++i;
      } else {
        std::this_thread::yield();
      }
    }
  }
  std::printf("SPSC: %llu ticks, %llu out of order\n",
              static_cast<unsigned long long>(kTicks),
              static_cast<unsigned long long>(out_of_order));

  ch07::MpmcBoundedQueue<int> orders(256);
  std::atomic<long long> consumed{0};
  {
    std::vector<std::jthread> ts;
    for (int p = 0; p < 2; ++p) {
      ts.emplace_back([&] {
        for (int i = 0; i < 50'000;) {
          if (orders.TryPush(1)) {
            ++i;
          } else {
            std::this_thread::yield();
          }
        }
      });
    }
    for (int c = 0; c < 2; ++c) {
      ts.emplace_back([&] {
        while (consumed.load() < 100'000) {
          if (auto o = orders.TryPop()) {
            consumed += *o;
          } else {
            std::this_thread::yield();
          }
        }
      });
    }
  }
  std::printf("MPMC: consumed %lld orders\n", consumed.load());

  ch07::LockFreeStack<int> stack;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&] {
        for (int i = 0; i < 10'000; ++i) {
          stack.Push(i);
          (void)stack.Pop();
        }
      });
    }
  }
  std::printf("stack empty: %d, pending reclamation: %zu\n", stack.empty(),
              stack.pending_count());
}
