// Fixed, deterministic workloads for valgrind (memcheck, helgrind, drd,
// callgrind, cachegrind, massif). Sizes are small because valgrind
// serialises threads and runs 20-100x slower.
// Usage: williams_profile <workload>; run without arguments to list.

#include <cstdio>
#include <cstring>
#include <execution>
#include <functional>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "appendices/order_gateway.h"
#include "ch02_managing_threads/parallel_accumulate.h"
#include "ch03_sharing_data/account.h"
#include "ch03_sharing_data/quote_cache.h"
#include "ch04_synchronizing/futures.h"
#include "ch04_synchronizing/thread_safe_queue.h"
#include "ch05_memory_model/atomic_stats.h"
#include "ch05_memory_model/seq_lock.h"
#include "ch05_memory_model/spin_lock.h"
#include "ch06_lock_based_ds/fine_grained_queue.h"
#include "ch06_lock_based_ds/lookup_table.h"
#include "ch06_lock_based_ds/thread_safe_list.h"
#include "ch07_lock_free/lock_free_stack.h"
#include "ch07_lock_free/mpmc_queue.h"
#include "ch07_lock_free/spsc_ring_buffer.h"
#include "ch08_designing_concurrent_code/parallel_algorithms.h"
#include "ch09_advanced_thread_mgmt/thread_pool.h"
#include "ch10_parallel_algorithms/portfolio_algos.h"

namespace {

namespace w = cpp4q::williams;

double g_sink = 0.0;  // keeps results observable

std::vector<double> Normals(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::normal_distribution<double> z;
  std::vector<double> v(n);
  for (auto& x : v) x = z(rng);
  return v;
}

void Accumulate() {
  std::vector<w::ch02::Position> book(200'000, {10.0, 100.0, 101.0});
  g_sink += w::ch02::PortfolioPnl(book, 4);
}

void Transfers() {
  std::vector<std::unique_ptr<w::ch03::Account>> acc;
  for (int i = 0; i < 4; ++i)
    acc.push_back(std::make_unique<w::ch03::Account>(1000));
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&, t] {
        for (int i = 0; i < 5'000; ++i) {
          w::ch03::Transfer(*acc[static_cast<std::size_t>((i + t) % 4)],
                            *acc[static_cast<std::size_t>((i + t + 1) % 4)], 1);
        }
      });
    }
  }
  for (auto& a : acc) g_sink += static_cast<double>(a->Balance());
}

void Quotes() {
  w::ch03::QuoteCache cache;
  std::vector<std::string> syms;
  for (int i = 0; i < 32; ++i) syms.push_back("S" + std::to_string(i));
  for (auto& s : syms) cache.Update(s, {1.0, 1.1});
  {
    std::jthread writer([&] {
      for (int i = 0; i < 5'000; ++i) {
        cache.Update(syms[static_cast<std::size_t>(i % 32)],
                     {1.0 * i, 1.0 * i + 0.1});
      }
    });
    std::vector<std::jthread> readers;
    for (int r = 0; r < 3; ++r) {
      readers.emplace_back([&] {
        double s = 0;
        for (int i = 0; i < 20'000; ++i) {
          if (auto q = cache.Get(syms[static_cast<std::size_t>(i % 32)]))
            s += q->Mid();
        }
        std::atomic_ref<double>(g_sink).fetch_add(s);
      });
    }
  }
}

void Queue() {
  w::ch04::ThreadSafeQueue<int> q;
  std::atomic<long long> sum{0};
  {
    std::vector<std::jthread> consumers;
    for (int c = 0; c < 2; ++c) {
      consumers.emplace_back([&] {
        while (auto v = q.WaitAndPop()) sum += *v;
      });
    }
    {
      std::vector<std::jthread> producers;
      for (int p = 0; p < 2; ++p) {
        producers.emplace_back([&] {
          for (int i = 0; i < 20'000; ++i) q.Push(i);
        });
      }
    }
    q.Close();
  }
  g_sink += static_cast<double>(sum.load());
}

void Futures() {
  w::ch04::PricingService service;
  std::vector<std::future<double>> fs;
  for (int i = 0; i < 2'000; ++i)
    fs.push_back(service.Submit([i] { return 0.5 * i; }));
  for (auto& f : fs) g_sink += f.get();
  g_sink += w::ch04::HistoricalVarAsync(Normals(10'000, 1), 0.99).get();
}

void Sort() {
  auto v = Normals(200'000, 2);
  w::ch04::ParallelQuickSort(v);
  g_sink += v[v.size() / 2];
}

void Lockstep() {
  const auto m =
      w::ch04::LockstepMeanSpot(20'000, 52, 100.0, 0.2, 1.0 / 52, 4, 1);
  g_sink += m.back();
}

void SpinLockWl() {
  w::ch05::SpinLock lock;
  long long n = 0;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&] {
        for (int i = 0; i < 20'000; ++i) {
          std::lock_guard g(lock);
          ++n;
        }
      });
    }
  }
  g_sink += static_cast<double>(n);
}

void SeqLockWl() {
  w::ch05::SeqLock<w::ch05::TopOfBook> book;
  std::atomic<bool> done{false};
  {
    std::jthread reader([&] {
      double s = 0;
      while (!done.load()) s += book.Load().bid;
      std::atomic_ref<double>(g_sink).fetch_add(s > 0 ? 1.0 : 0.0);
    });
    for (std::int64_t i = 0; i < 50'000; ++i) {
      book.Store({1.0 * static_cast<double>(i), 1.0, i, i});
    }
    done = true;
  }
}

void Histogram() {
  const auto pnl = Normals(400'000, 3);
  const auto h = w::ch05::ParallelPnlHistogram(pnl, -4.0, 4.0, 64, 4);
  g_sink += static_cast<double>(h[32]);
}

void Lookup() {
  w::ch06::ThreadSafeLookupTable<int, long long> t(31);
  {
    std::vector<std::jthread> ts;
    for (int th = 0; th < 4; ++th) {
      ts.emplace_back([&] {
        for (int i = 0; i < 20'000; ++i)
          t.Modify(i % 200, [](long long& v) { ++v; });
      });
    }
  }
  g_sink += static_cast<double>(t.Snapshot().size());
}

void List() {
  w::ch06::ThreadSafeList<int> list;
  {
    std::jthread remover([&] {
      for (int i = 0; i < 50; ++i)
        list.RemoveIf([](int x) { return x % 5 == 0; });
    });
    std::jthread writer([&] {
      for (int i = 0; i < 5'000; ++i) list.PushFront(i);
    });
  }
  list.ForEach([](int x) { g_sink += x; });
}

void TwoLockQueue() {
  w::ch06::FineGrainedQueue<int> q;
  long long sum = 0;
  {
    std::jthread consumer([&] {
      for (int i = 0; i < 40'000; ++i) sum += q.WaitAndPop();
    });
    for (int i = 0; i < 40'000; ++i) q.Push(i);
  }
  g_sink += static_cast<double>(sum);
}

void Spsc() {
  w::ch07::SpscRingBuffer<int, 1024> rb;
  long long sum = 0;
  {
    std::jthread consumer([&] {
      for (int got = 0; got < 100'000;) {
        if (auto v = rb.TryPop()) {
          sum += *v;
          ++got;
        } else {
          std::this_thread::yield();
        }
      }
    });
    for (int i = 0; i < 100'000;) {
      if (rb.TryPush(i)) {
        ++i;
      } else {
        std::this_thread::yield();
      }
    }
  }
  g_sink += static_cast<double>(sum);
}

void Mpmc() {
  w::ch07::MpmcBoundedQueue<int> q(256);
  std::atomic<int> consumed{0};
  {
    std::vector<std::jthread> ts;
    for (int p = 0; p < 2; ++p) {
      ts.emplace_back([&] {
        for (int i = 0; i < 20'000;) {
          if (q.TryPush(i)) {
            ++i;
          } else {
            std::this_thread::yield();
          }
        }
      });
      ts.emplace_back([&] {
        while (consumed.load() < 40'000) {
          if (q.TryPop()) {
            ++consumed;
          } else {
            std::this_thread::yield();
          }
        }
      });
    }
  }
  g_sink += consumed.load();
}

void Stack() {
  w::ch07::LockFreeStack<int> s;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&] {
        for (int i = 0; i < 10'000; ++i) {
          s.Push(i);
          if (auto v = s.Pop()) std::atomic_ref<double>(g_sink).fetch_add(*v);
        }
      });
    }
  }
  while (s.Pop()) {
  }
}

void ScanFind() {
  const auto v = Normals(1'000'000, 4);
  std::vector<double> out(v.size());
  w::ch08::ParallelInclusiveScan<double>(v, out, 4);
  g_sink += out.back();
  const auto it = w::ch08::ParallelFind(
      v.begin(), v.end(), [](double x) { return x > 4.5; }, 4);
  g_sink += static_cast<double>(it - v.begin());
}

void FalseSharing() {
  std::vector<std::int32_t> ids(1'000'000);
  std::iota(ids.begin(), ids.end(), 0);
  g_sink += static_cast<double>(w::ch08::CountByBucketPacked(ids, 4)[0]);
  g_sink += static_cast<double>(w::ch08::CountByBucketPadded(ids, 4)[0]);
}

void Var() {
  const std::vector<double> exposures(50, 1e5);
  const auto scen = Normals(50 * 10'000, 5);
  g_sink += w::ch08::ScenarioVar(exposures, scen, 0.99, 4);
}

void Pool() {
  w::ch09::ThreadPool pool(4);
  std::vector<std::future<int>> fs;
  for (int i = 0; i < 5'000; ++i) fs.push_back(pool.Submit([i] { return i; }));
  for (auto& f : fs) g_sink += f.get();
  auto v = Normals(200'000, 6);
  w::ch09::PoolQuickSort(pool, v);
  g_sink += v.front();
}

void Pstl() {
  const auto book = w::ch10::MakeBondBook(200'000, 1);
  g_sink += w::ch10::PortfolioPv(std::execution::par, book, 0.04);
  auto copy = book;
  w::ch10::SortByNotional(std::execution::par, copy);
  g_sink += copy.front().notional;
}

void Gateway() {
  namespace ac = w::appendix_c;
  ac::ClientMailbox client;
  ac::OrderGateway gw(client);
  for (std::int64_t id = 1; id <= 2'000; ++id) {
    gw.mailbox().Send(ac::NewOrder{id, 100, id % 2 ? 101.0 : 99.0});
  }
  // marketable: PendingNew, Live, Partial, Filled; resting: PendingNew, Live.
  for (int i = 0; i < 1'000 * 4 + 1'000 * 2; ++i) {
    g_sink += static_cast<double>(
        std::get<ac::ExecutionReport>(client.Receive()).filled);
  }
}

struct Workload {
  std::string_view name;
  void (*fn)();
};
constexpr Workload kWorkloads[] = {
    {"accumulate", Accumulate},
    {"transfers", Transfers},
    {"quotes", Quotes},
    {"queue", Queue},
    {"futures", Futures},
    {"sort", Sort},
    {"lockstep", Lockstep},
    {"spinlock", SpinLockWl},
    {"seqlock", SeqLockWl},
    {"histogram", Histogram},
    {"lookup", Lookup},
    {"list", List},
    {"twolock", TwoLockQueue},
    {"spsc", Spsc},
    {"mpmc", Mpmc},
    {"stack", Stack},
    {"scanfind", ScanFind},
    {"falseshare", FalseSharing},
    {"var", Var},
    {"pool", Pool},
    {"pstl", Pstl},
    {"gateway", Gateway},
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::puts("usage: williams_profile <workload|all>");
    for (const auto& w : kWorkloads)
      std::printf("  %.*s\n", static_cast<int>(w.name.size()), w.name.data());
    return 1;
  }
  const std::string_view which = argv[1];
  bool ran = false;
  for (const auto& w : kWorkloads) {
    if (which == "all" || which == w.name) {
      w.fn();
      ran = true;
    }
  }
  if (!ran) {
    std::fprintf(stderr, "unknown workload %s\n", argv[1]);
    return 1;
  }
  std::printf("%g\n", g_sink);
}
