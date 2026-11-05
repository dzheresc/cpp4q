#include <gtest/gtest.h>

#include <atomic>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#include "ch02_managing_threads/parallel_accumulate.h"
#include "ch02_managing_threads/thread_guards.h"

namespace cpp4q::williams::ch02 {
namespace {

TEST(ThreadGuardsTest, ScopedThreadJoinsOnScopeExit) {
  std::atomic<bool> ran{false};
  {
    ScopedThread t(std::thread([&] { ran = true; }));
  }
  EXPECT_TRUE(ran.load());
}

TEST(ThreadGuardsTest, ScopedThreadRejectsEmptyThread) {
  EXPECT_THROW(ScopedThread(std::thread{}), std::logic_error);
}

TEST(ThreadGuardsTest, JoiningThreadMoveAssignJoinsOldThread) {
  std::atomic<int> count{0};
  JoiningThread a([&] { ++count; });
  JoiningThread b([&] { ++count; });
  a = std::move(b);  // joins a's original thread first
  EXPECT_GE(count.load(), 1);
  EXPECT_FALSE(b.joinable());
  a.Join();
  EXPECT_EQ(count.load(), 2);
}

TEST(ParallelAccumulateTest, MatchesStdAccumulate) {
  std::vector<long long> v(100'003);
  std::iota(v.begin(), v.end(), 1);
  for (unsigned threads : {1u, 2u, 3u, 8u}) {
    EXPECT_EQ(ParallelAccumulate(v.begin(), v.end(), 0LL, 10, threads),
              std::accumulate(v.begin(), v.end(), 0LL))
        << threads << " threads";
  }
}

TEST(ParallelAccumulateTest, EmptyRangeReturnsInit) {
  std::vector<int> v;
  EXPECT_EQ(ParallelAccumulate(v.begin(), v.end(), 5), 5);
}

TEST(ParallelAccumulateTest, ChooseThreadCount) {
  EXPECT_EQ(ChooseThreadCount(10, 25, 8), 1u);         // too little work
  EXPECT_EQ(ChooseThreadCount(100, 25, 8), 4u);        // 100/25
  EXPECT_EQ(ChooseThreadCount(1'000'000, 25, 8), 8u);  // capped
  EXPECT_EQ(ChooseThreadCount(0, 25, 8), 1u);
}

TEST(ParallelAccumulateTest, PortfolioPnl) {
  std::vector<Position> book(
      50'000, {.quantity = 10.0, .entry_price = 100.0, .mark_price = 100.5});
  EXPECT_NEAR(PortfolioPnl(book, 4), 50'000 * 5.0, 1e-6);
}

}  // namespace
}  // namespace cpp4q::williams::ch02
