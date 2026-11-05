// Chapters 4, 6, 7: the same producer/consumer hand-off through each queue
// in the book, plus the lock-free stack against a mutex-guarded vector.
//
// Queue benchmarks move kItems integers from `P` producer threads to `P`
// consumer threads per iteration; items/s is the hand-off throughput.

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "ch04_synchronizing/thread_safe_queue.h"
#include "ch06_lock_based_ds/fine_grained_queue.h"
#include "ch07_lock_free/lock_free_stack.h"
#include "ch07_lock_free/mpmc_queue.h"
#include "ch07_lock_free/spsc_ring_buffer.h"

namespace {

namespace ch04 = cpp4q::williams::ch04;
namespace ch06 = cpp4q::williams::ch06;
namespace ch07 = cpp4q::williams::ch07;

constexpr int kItems = 1 << 18;

// push(i) must eventually succeed; pop() returns true and adds to sum.
template <typename Push, typename Pop>
void RunHandOff(benchmark::State& state, int pairs, Push push, Pop pop) {
  for (auto _ : state) {
    std::atomic<std::int64_t> sum{0};
    {
      std::vector<std::jthread> ts;
      const int per = kItems / pairs;
      for (int p = 0; p < pairs; ++p) {
        ts.emplace_back([&] {
          for (int i = 0; i < per; ++i) push(i);
        });
        ts.emplace_back([&] {
          std::int64_t local = 0;
          for (int i = 0; i < per; ++i) local += pop();
          sum += local;
        });
      }
    }
    benchmark::DoNotOptimize(sum.load());
  }
  state.SetItemsProcessed(state.iterations() * kItems);
}

void BM_HandOff_MutexCvQueue(benchmark::State& state) {
  ch04::ThreadSafeQueue<int> q;
  RunHandOff(
      state, static_cast<int>(state.range(0)), [&](int i) { q.Push(i); },
      [&] { return *q.WaitAndPop(); });
}
BENCHMARK(BM_HandOff_MutexCvQueue)
    ->Arg(1)
    ->Arg(2)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

void BM_HandOff_TwoLockQueue(benchmark::State& state) {
  ch06::FineGrainedQueue<int> q;
  RunHandOff(
      state, static_cast<int>(state.range(0)), [&](int i) { q.Push(i); },
      [&] { return q.WaitAndPop(); });
}
BENCHMARK(BM_HandOff_TwoLockQueue)
    ->Arg(1)
    ->Arg(2)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

void BM_HandOff_MpmcLockFree(benchmark::State& state) {
  ch07::MpmcBoundedQueue<int> q(1024);
  RunHandOff(
      state, static_cast<int>(state.range(0)),
      [&](int i) {
        while (!q.TryPush(i)) std::this_thread::yield();
      },
      [&] {
        for (;;) {
          if (auto v = q.TryPop()) return *v;
          std::this_thread::yield();
        }
      });
}
BENCHMARK(BM_HandOff_MpmcLockFree)
    ->Arg(1)
    ->Arg(2)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

void BM_HandOff_SpscRing(benchmark::State& state) {
  ch07::SpscRingBuffer<int, 1024> q;
  RunHandOff(
      state, 1,
      [&](int i) {
        while (!q.TryPush(i)) std::this_thread::yield();
      },
      [&] {
        for (;;) {
          if (auto v = q.TryPop()) return *v;
          std::this_thread::yield();
        }
      });
}
BENCHMARK(BM_HandOff_SpscRing)->UseRealTime()->Unit(benchmark::kMillisecond);

ch07::LockFreeStack<int> g_lf_stack;
void BM_StackPushPop_LockFree(benchmark::State& state) {
  for (auto _ : state) {
    g_lf_stack.Push(1);
    benchmark::DoNotOptimize(g_lf_stack.Pop());
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StackPushPop_LockFree)->ThreadRange(1, 4)->UseRealTime();

std::mutex g_stack_mutex;
std::vector<int> g_stack;
void BM_StackPushPop_Mutex(benchmark::State& state) {
  for (auto _ : state) {
    {
      std::lock_guard lock(g_stack_mutex);
      g_stack.push_back(1);
    }
    std::lock_guard lock(g_stack_mutex);
    if (!g_stack.empty()) {
      benchmark::DoNotOptimize(g_stack.back());
      g_stack.pop_back();
    }
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StackPushPop_Mutex)->ThreadRange(1, 4)->UseRealTime();

}  // namespace
