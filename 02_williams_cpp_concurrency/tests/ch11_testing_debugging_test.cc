#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <thread>

#include "ch11_testing_debugging/stress_harness.h"

namespace cpp4q::williams::ch11 {
namespace {

TEST(StressHarnessTest, RunsEveryThreadIndexOnce) {
  std::atomic<unsigned> mask{0};
  RunConcurrently(5, [&](unsigned i) { mask.fetch_or(1U << i); });
  EXPECT_EQ(mask.load(), 0b11111U);
}

TEST(StressHarnessTest, ThreadsReallyOverlap) {
  // All threads must be alive at once for this to finish: each waits until
  // it has seen every other thread arrive.
  std::atomic<int> arrived{0};
  RunConcurrently(4, [&](unsigned) {
    ++arrived;
    while (arrived.load() < 4) std::this_thread::yield();
  });
  EXPECT_EQ(arrived.load(), 4);
}

TEST(StressHarnessTest, RethrowsWorkerException) {
  EXPECT_THROW(RunConcurrently(3,
                               [](unsigned i) {
                                 if (i == 1) throw std::runtime_error("boom");
                               }),
               std::runtime_error);
}

TEST(PreTradeLimitTest, CasReserveNeverBreachesLimit) {
  PreTradeLimit limit(1'000);
  const int failures = CountFailingRounds(
      50, 8, [&] { limit.Reset(); },
      [&](unsigned) {
        for (int i = 0; i < 50; ++i) (void)limit.TryReserve(7);
      },
      [&] { return limit.used() <= limit.limit(); });
  EXPECT_EQ(failures, 0);
}

TEST(PreTradeLimitTest, CasReserveFillsToCapacity) {
  // 8 threads x 50 x 7 = 2800 demand against 1000 limit; CAS reserve must
  // fill exactly floor(1000/7)*7 = 994.
  PreTradeLimit limit(1'000);
  RunConcurrently(8, [&](unsigned) {
    for (int i = 0; i < 50; ++i) (void)limit.TryReserve(7);
  });
  EXPECT_EQ(limit.used(), 994);
  limit.Release(994);
  EXPECT_EQ(limit.used(), 0);
}

TEST(PreTradeLimitTest, NaiveSingleThreadedIsFine) {
  // The naive version is only wrong under concurrency.
  PreTradeLimit limit(10);
  EXPECT_TRUE(limit.TryReserveNaive(6));
  EXPECT_FALSE(limit.TryReserveNaive(6));
  EXPECT_EQ(limit.used(), 6);
}

}  // namespace
}  // namespace cpp4q::williams::ch11
