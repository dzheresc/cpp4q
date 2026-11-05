#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ch04_synchronizing/futures.h"
#include "ch04_synchronizing/thread_safe_queue.h"

namespace cpp4q::williams::ch04 {
namespace {

TEST(ThreadSafeQueueTest, FifoAndTryPop) {
  ThreadSafeQueue<int> q;
  EXPECT_FALSE(q.TryPop().has_value());
  q.Push(1);
  q.Push(2);
  EXPECT_EQ(q.size(), 2u);
  EXPECT_EQ(*q.TryPop(), 1);
  EXPECT_EQ(*q.WaitAndPop(), 2);
  EXPECT_TRUE(q.empty());
}

TEST(ThreadSafeQueueTest, CloseWakesWaitersAndDrains) {
  ThreadSafeQueue<int> q;
  q.Push(7);
  q.Close();
  EXPECT_FALSE(q.Push(8));
  EXPECT_EQ(*q.WaitAndPop(), 7);             // drains what is left
  EXPECT_FALSE(q.WaitAndPop().has_value());  // then reports closed
}

TEST(ThreadSafeQueueTest, StopTokenCancelsWait) {
  ThreadSafeQueue<int> q;
  std::atomic<bool> returned_empty{false};
  std::jthread waiter([&](std::stop_token st) {
    returned_empty = !q.WaitAndPop(st).has_value();
  });
  waiter.request_stop();
  waiter.join();
  EXPECT_TRUE(returned_empty.load());
}

TEST(ThreadSafeQueueTest, ManyProducersManyConsumersLoseNothing) {
  ThreadSafeQueue<int> q;
  constexpr int kPerProducer = 20'000, kProducers = 3, kConsumers = 3;
  std::atomic<long long> sum{0};
  std::atomic<int> count{0};
  {
    std::vector<std::jthread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
      consumers.emplace_back([&] {
        while (auto v = q.WaitAndPop()) {
          sum += *v;
          ++count;
        }
      });
    }
    {
      std::vector<std::jthread> producers;
      for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&] {
          for (int i = 1; i <= kPerProducer; ++i) q.Push(i);
        });
      }
    }
    q.Close();
  }
  EXPECT_EQ(count.load(), kProducers * kPerProducer);
  EXPECT_EQ(sum.load(), kProducers * static_cast<long long>(kPerProducer) *
                            (kPerProducer + 1) / 2);
}

TEST(PricingServiceTest, ReturnsResultsAndExceptions) {
  PricingService service;
  auto a = service.Submit([] { return 1.5; });
  auto b = service.Submit([]() -> double { throw std::domain_error("x"); });
  auto c = service.Submit([] { return 2.5; });
  EXPECT_DOUBLE_EQ(a.get(), 1.5);
  EXPECT_THROW((void)b.get(), std::domain_error);
  EXPECT_DOUBLE_EQ(c.get(), 2.5);
}

TEST(PricingServiceTest, DestructorRunsQueuedJobs) {
  std::vector<std::future<double>> results;
  {
    PricingService service;
    for (int i = 0; i < 100; ++i) {
      results.push_back(service.Submit([i] { return 1.0 * i; }));
    }
  }
  double total = 0.0;
  for (auto& f : results) total += f.get();
  EXPECT_DOUBLE_EQ(total, 4950.0);
}

TEST(HistoricalVarAsyncTest, ValueAndError) {
  std::vector<double> pnl(100);
  std::iota(pnl.begin(), pnl.end(), -50.0);  // -50..49
  EXPECT_DOUBLE_EQ(HistoricalVarAsync(pnl, 0.95).get(), 45.0);
  EXPECT_THROW((void)HistoricalVarAsync({}, 0.95).get(), std::invalid_argument);
  EXPECT_THROW((void)HistoricalVarAsync(pnl, 1.5).get(), std::invalid_argument);
}

TEST(ParallelQuickSortTest, SortsLikeStdSort) {
  std::mt19937 rng(3);
  std::uniform_int_distribution<int> d(0, 50);  // many duplicates
  for (std::size_t n : {0u, 1u, 17u, 10'000u, 200'000u}) {
    std::vector<int> v(n);
    for (auto& x : v) x = d(rng);
    auto expected = v;
    std::sort(expected.begin(), expected.end());
    ParallelQuickSort(v, std::less<>{}, 512);
    EXPECT_EQ(v, expected) << n;
  }
}

TEST(ParallelQuickSortTest, CustomComparator) {
  std::vector<double> v{3, 1, 2, 5, 4};
  ParallelQuickSort(v, std::greater<>{}, 1);
  EXPECT_EQ(v, (std::vector<double>{5, 4, 3, 2, 1}));
}

TEST(LockstepTest, DeterministicAndMartingale) {
  const auto a = LockstepMeanSpot(40'000, 12, 100.0, 0.2, 1.0 / 12, 4, 7);
  const auto b = LockstepMeanSpot(40'000, 12, 100.0, 0.2, 1.0 / 12, 4, 7);
  ASSERT_EQ(a.size(), 12u);
  EXPECT_EQ(a, b);
  // E[S_t] = S_0 under zero drift; sd of the mean ~ 100*0.2/sqrt(40000).
  EXPECT_NEAR(a.back(), 100.0, 0.5);
}

TEST(LockstepTest, SingleWorkerWorks) {
  const auto m = LockstepMeanSpot(1'000, 3, 50.0, 0.1, 0.25, 1, 1);
  ASSERT_EQ(m.size(), 3u);
  for (double x : m) EXPECT_TRUE(std::isfinite(x));
}

}  // namespace
}  // namespace cpp4q::williams::ch04
