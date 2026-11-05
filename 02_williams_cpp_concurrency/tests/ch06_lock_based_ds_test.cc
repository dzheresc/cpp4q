#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "ch06_lock_based_ds/fine_grained_queue.h"
#include "ch06_lock_based_ds/lookup_table.h"
#include "ch06_lock_based_ds/thread_safe_list.h"

namespace cpp4q::williams::ch06 {
namespace {

TEST(FineGrainedQueueTest, Fifo) {
  FineGrainedQueue<std::string> q;
  EXPECT_TRUE(q.empty());
  EXPECT_FALSE(q.TryPop().has_value());
  q.Push("a");
  q.Push("b");
  EXPECT_FALSE(q.empty());
  EXPECT_EQ(*q.TryPop(), "a");
  EXPECT_EQ(q.WaitAndPop(), "b");
  EXPECT_TRUE(q.empty());
}

TEST(FineGrainedQueueTest, MoveOnlyPayload) {
  FineGrainedQueue<std::unique_ptr<int>> q;
  q.Push(std::make_unique<int>(4));
  EXPECT_EQ(*q.WaitAndPop(), 4);
}

TEST(FineGrainedQueueTest, ProducersAndBlockingConsumers) {
  FineGrainedQueue<int> q;
  constexpr int kN = 30'000;
  std::atomic<long long> sum{0};
  {
    std::vector<std::jthread> ts;
    for (int c = 0; c < 2; ++c) {
      ts.emplace_back([&] {
        for (int i = 0; i < kN; ++i) sum += q.WaitAndPop();
      });
    }
    for (int p = 0; p < 2; ++p) {
      ts.emplace_back([&] {
        for (int i = 1; i <= kN; ++i) q.Push(i);
      });
    }
  }
  EXPECT_EQ(sum.load(), 2LL * kN * (kN + 1) / 2);
  EXPECT_TRUE(q.empty());
}

TEST(FineGrainedQueueTest, LongQueueDestructsWithoutStackOverflow) {
  auto q = std::make_unique<FineGrainedQueue<int>>();
  for (int i = 0; i < 1'000'000; ++i) q->Push(i);
  q.reset();
  SUCCEED();
}

TEST(LookupTableTest, AddUpdateRemove) {
  ThreadSafeLookupTable<std::string, int> t(7);
  EXPECT_EQ(t.ValueFor("AAPL", -1), -1);
  t.AddOrUpdate("AAPL", 100);
  t.AddOrUpdate("AAPL", 150);
  t.AddOrUpdate("MSFT", 50);
  EXPECT_EQ(t.ValueFor("AAPL"), 150);
  EXPECT_TRUE(t.Remove("AAPL"));
  EXPECT_FALSE(t.Remove("AAPL"));
  const auto snap = t.Snapshot();
  ASSERT_EQ(snap.size(), 1u);
  EXPECT_EQ(snap.at("MSFT"), 50);
}

TEST(LookupTableTest, ConcurrentModifyIsAtomic) {
  ThreadSafeLookupTable<int, long long> t(5);
  {
    std::vector<std::jthread> ts;
    for (int th = 0; th < 4; ++th) {
      ts.emplace_back([&] {
        for (int i = 0; i < 20'000; ++i) {
          t.Modify(i % 13, [](long long& v) { ++v; });
        }
      });
    }
  }
  long long total = 0;
  for (const auto& [k, v] : t.Snapshot()) total += v;
  EXPECT_EQ(total, 80'000);
  EXPECT_EQ(t.Snapshot().size(), 13u);
}

TEST(ThreadSafeListTest, PushFindRemoveForEach) {
  ThreadSafeList<int> list;
  for (int i = 0; i < 10; ++i) list.PushFront(i);
  EXPECT_EQ(*list.FindFirstIf([](int x) { return x < 5; }), 4);
  EXPECT_FALSE(list.FindFirstIf([](int x) { return x > 100; }).has_value());
  EXPECT_EQ(list.RemoveIf([](int x) { return x % 2 == 0; }), 5u);
  std::vector<int> seen;
  list.ForEach([&](int x) { seen.push_back(x); });
  EXPECT_EQ(seen, (std::vector<int>{9, 7, 5, 3, 1}));
}

TEST(ThreadSafeListTest, ConcurrentPushRemoveAndTraverse) {
  ThreadSafeList<int> list;
  std::atomic<bool> done{false};
  {
    std::jthread reader([&] {
      while (!done) {
        long long s = 0;
        list.ForEach([&](int x) { s += x; });
        (void)s;
      }
    });
    std::jthread remover([&] {
      while (!done) list.RemoveIf([](int x) { return x % 3 == 0; });
    });
    {
      std::vector<std::jthread> writers;
      for (int w = 0; w < 2; ++w) {
        writers.emplace_back([&] {
          for (int i = 0; i < 5'000; ++i) list.PushFront(i);
        });
      }
    }
    done = true;
  }
  list.RemoveIf([](int x) { return x % 3 == 0; });
  int count = 0;
  list.ForEach([&](int x) {
    EXPECT_NE(x % 3, 0);
    ++count;
  });
  // 2 writers x (5000 - 1667 multiples of 3).
  EXPECT_EQ(count, 2 * (5'000 - 1'667));
}

}  // namespace
}  // namespace cpp4q::williams::ch06
