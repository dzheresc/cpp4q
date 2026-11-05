#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ch07_lock_free/lock_free_stack.h"
#include "ch07_lock_free/mpmc_queue.h"
#include "ch07_lock_free/spsc_ring_buffer.h"

namespace cpp4q::williams::ch07 {
namespace {

TEST(SpscRingBufferTest, FullAndEmpty) {
  SpscRingBuffer<int, 4> rb;
  EXPECT_FALSE(rb.TryPop().has_value());
  for (int i = 0; i < 4; ++i) EXPECT_TRUE(rb.TryPush(i));
  EXPECT_FALSE(rb.TryPush(99));
  EXPECT_EQ(rb.size(), 4u);
  for (int i = 0; i < 4; ++i) EXPECT_EQ(*rb.TryPop(), i);
  EXPECT_FALSE(rb.TryPop().has_value());
}

TEST(SpscRingBufferTest, WrapsAround) {
  SpscRingBuffer<int, 8> rb;
  for (int i = 0; i < 1000; ++i) {
    ASSERT_TRUE(rb.TryPush(i));
    ASSERT_EQ(*rb.TryPop(), i);
  }
}

TEST(SpscRingBufferTest, ProducerConsumerInOrder) {
  SpscRingBuffer<std::uint64_t, 256> rb;
  constexpr std::uint64_t kN = 500'000;
  bool in_order = true;
  {
    std::jthread consumer([&] {
      for (std::uint64_t expected = 0; expected < kN;) {
        if (auto v = rb.TryPop()) {
          if (*v != expected) in_order = false;
          ++expected;
        } else {
          std::this_thread::yield();  // keeps valgrind's serialised threads
                                      // moving
        }
      }
    });
    for (std::uint64_t i = 0; i < kN;) {
      if (rb.TryPush(i)) {
        ++i;
      } else {
        std::this_thread::yield();
      }
    }
  }
  EXPECT_TRUE(in_order);
}

TEST(MpmcQueueTest, RejectsBadCapacity) {
  EXPECT_THROW(MpmcBoundedQueue<int>(3), std::invalid_argument);
  EXPECT_THROW(MpmcBoundedQueue<int>(1), std::invalid_argument);
}

TEST(MpmcQueueTest, FifoSingleThreaded) {
  MpmcBoundedQueue<int> q(4);
  for (int i = 0; i < 4; ++i) EXPECT_TRUE(q.TryPush(i));
  EXPECT_FALSE(q.TryPush(4));
  for (int i = 0; i < 4; ++i) EXPECT_EQ(*q.TryPop(), i);
  EXPECT_FALSE(q.TryPop().has_value());
}

TEST(MpmcQueueTest, DestroysRemainingItems) {
  auto counter = std::make_shared<int>(0);
  {
    MpmcBoundedQueue<std::shared_ptr<int>> q(8);
    q.TryPush(counter);
    q.TryPush(counter);
    EXPECT_EQ(counter.use_count(), 3);
  }
  EXPECT_EQ(counter.use_count(), 1);
}

TEST(MpmcQueueTest, EveryItemDeliveredExactlyOnce) {
  MpmcBoundedQueue<int> q(64);
  constexpr int kPerProducer = 50'000, kProducers = 2, kConsumers = 2;
  std::vector<std::atomic<int>> seen(kPerProducer * kProducers);
  std::atomic<int> consumed{0};
  {
    std::vector<std::jthread> ts;
    for (int p = 0; p < kProducers; ++p) {
      ts.emplace_back([&, p] {
        for (int i = 0; i < kPerProducer;) {
          if (q.TryPush(p * kPerProducer + i)) {
            ++i;
          } else {
            std::this_thread::yield();
          }
        }
      });
    }
    for (int c = 0; c < kConsumers; ++c) {
      ts.emplace_back([&] {
        while (consumed.load() < kPerProducer * kProducers) {
          if (auto v = q.TryPop()) {
            seen[static_cast<std::size_t>(*v)].fetch_add(1);
            consumed.fetch_add(1);
          } else {
            std::this_thread::yield();
          }
        }
      });
    }
  }
  EXPECT_TRUE(std::all_of(seen.begin(), seen.end(),
                          [](const auto& s) { return s.load() == 1; }));
}

TEST(LockFreeStackTest, Lifo) {
  LockFreeStack<int> s;
  EXPECT_TRUE(s.empty());
  EXPECT_FALSE(s.Pop().has_value());
  s.Push(1);
  s.Push(2);
  EXPECT_EQ(*s.Pop(), 2);
  EXPECT_EQ(*s.Pop(), 1);
  EXPECT_TRUE(s.empty());
}

TEST(LockFreeStackTest, ConcurrentPushPopConservesItems) {
  LockFreeStack<int> s;
  constexpr int kThreads = 4, kOps = 20'000;
  std::atomic<long long> popped_sum{0};
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < kThreads; ++t) {
      ts.emplace_back([&] {
        for (int i = 1; i <= kOps; ++i) {
          s.Push(i);
          if (auto v = s.Pop()) popped_sum += *v;
        }
      });
    }
  }
  long long rest = 0;
  while (auto v = s.Pop()) rest += *v;
  EXPECT_EQ(popped_sum.load() + rest,
            kThreads * static_cast<long long>(kOps) * (kOps + 1) / 2);
  // Single-threaded pops above found threads_in_pop == 1 and drained the
  // pending list.
  EXPECT_EQ(s.pending_count(), 0u);
}

TEST(LockFreeStackTest, NonTrivialPayloadIsDestroyed) {
  auto token = std::make_shared<int>(1);
  {
    LockFreeStack<std::shared_ptr<int>> s;
    s.Push(token);
    s.Push(token);
    (void)s.Pop();
    EXPECT_EQ(token.use_count(), 2);
  }
  EXPECT_EQ(token.use_count(), 1);
}

}  // namespace
}  // namespace cpp4q::williams::ch07
