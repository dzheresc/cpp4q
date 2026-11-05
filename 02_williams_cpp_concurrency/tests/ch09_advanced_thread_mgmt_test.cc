#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include "ch09_advanced_thread_mgmt/thread_pool.h"

namespace cpp4q::williams::ch09 {
namespace {

TEST(TaskTest, MoveOnlyCallable) {
  auto p = std::make_unique<int>(5);
  int out = 0;
  Task t([p = std::move(p), &out] { out = *p; });
  ASSERT_TRUE(static_cast<bool>(t));
  t();
  EXPECT_EQ(out, 5);
}

TEST(ThreadPoolTest, ReturnsValuesAndExceptions) {
  ThreadPool pool(3);
  EXPECT_EQ(pool.size(), 3u);
  auto a = pool.Submit([] { return 6 * 7; });
  auto b = pool.Submit([]() -> int { throw std::out_of_range("x"); });
  auto c = pool.Submit([] {});
  EXPECT_EQ(a.get(), 42);
  EXPECT_THROW(b.get(), std::out_of_range);
  c.get();
}

TEST(ThreadPoolTest, ManySmallTasks) {
  ThreadPool pool(4);
  std::atomic<int> count{0};
  std::vector<std::future<void>> fs;
  for (int i = 0; i < 10'000; ++i) fs.push_back(pool.Submit([&] { ++count; }));
  for (auto& f : fs) f.get();
  EXPECT_EQ(count.load(), 10'000);
}

TEST(ThreadPoolTest, NestedSubmitAndWaitDoesNotDeadlock) {
  // One worker: a task that waits on a sub-task would deadlock a naive pool.
  ThreadPool pool(1);
  auto outer = pool.Submit([&pool] {
    auto inner = pool.Submit([] { return 10; });
    return pool.WaitFor(inner) + 1;
  });
  EXPECT_EQ(outer.get(), 11);
}

TEST(ThreadPoolTest, DestructorFinishesQueuedTasks) {
  std::atomic<int> count{0};
  {
    ThreadPool pool(2);
    for (int i = 0; i < 1000; ++i) (void)pool.Submit([&] { ++count; });
  }
  EXPECT_EQ(count.load(), 1000);
}

TEST(PoolQuickSortTest, SortsLikeStdSort) {
  ThreadPool pool(4);
  std::mt19937 rng(4);
  std::uniform_int_distribution<int> d(0, 1000);
  for (std::size_t n : {0u, 10u, 50'000u, 300'000u}) {
    std::vector<int> v(n);
    for (auto& x : v) x = d(rng);
    auto expected = v;
    std::sort(expected.begin(), expected.end());
    PoolQuickSort(pool, v, 1024);
    EXPECT_EQ(v, expected) << n;
  }
}

TEST(BackgroundRepricerTest, RepricesAndStops) {
  BackgroundRepricer r(10.0);
  r.OnSpot(100.0);
  EXPECT_DOUBLE_EQ(r.WaitForValue(1), 1000.0);
  r.OnSpot(101.0);
  r.OnSpot(102.0);
  EXPECT_DOUBLE_EQ(r.WaitForValue(3), 1020.0);
  r.Stop();
  // After stop the waiter is released instead of hanging.
  (void)r.WaitForValue(100);
  EXPECT_TRUE(r.stopped());
}

}  // namespace
}  // namespace cpp4q::williams::ch09
