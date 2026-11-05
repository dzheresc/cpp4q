#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "ch05_memory_model/atomic_stats.h"
#include "ch05_memory_model/seq_lock.h"
#include "ch05_memory_model/spin_lock.h"

namespace cpp4q::williams::ch05 {
namespace {

TEST(SpinLockTest, MutualExclusion) {
  SpinLock lock;
  long long counter = 0;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&] {
        for (int i = 0; i < 50'000; ++i) {
          std::lock_guard g(lock);
          ++counter;
        }
      });
    }
  }
  EXPECT_EQ(counter, 200'000);
}

TEST(SpinLockTest, TryLock) {
  SpinLock lock;
  EXPECT_TRUE(lock.try_lock());
  EXPECT_FALSE(lock.try_lock());
  lock.unlock();
  EXPECT_TRUE(lock.try_lock());
  lock.unlock();
}

TEST(SeqLockTest, SingleThreadRoundTrip) {
  SeqLock<TopOfBook> s;
  EXPECT_EQ(s.version(), 1u);  // the constructor stores once
  s.Store({1.5, 1.6, 10, 20});
  const auto b = s.Load();
  EXPECT_DOUBLE_EQ(b.bid, 1.5);
  EXPECT_DOUBLE_EQ(b.ask, 1.6);
  EXPECT_EQ(b.bid_size, 10);
  EXPECT_EQ(b.ask_size, 20);
  EXPECT_EQ(s.version(), 2u);
}

TEST(SeqLockTest, OddSizedPayload) {
  struct Small {
    char c[3];
  };
  SeqLock<Small> s(Small{{'a', 'b', 'c'}});
  EXPECT_EQ(s.Load().c[2], 'c');
}

TEST(SeqLockTest, ReadersNeverSeeTornSnapshots) {
  SeqLock<TopOfBook> s(TopOfBook{0.0, 1.0, 0, 0});
  std::atomic<bool> done{false};
  std::atomic<long long> torn{0};
  {
    std::vector<std::jthread> readers;
    for (int r = 0; r < 3; ++r) {
      readers.emplace_back([&] {
        while (!done.load(std::memory_order_relaxed)) {
          const auto b = s.Load();
          // Writer keeps all four fields in a fixed relation.
          if (b.bid_size != b.ask_size || b.ask != b.bid + 1.0 ||
              b.bid != static_cast<double>(b.bid_size)) {
            ++torn;
          }
        }
      });
    }
    for (std::int64_t i = 1; i <= 200'000; ++i) {
      s.Store({static_cast<double>(i), static_cast<double>(i) + 1.0, i, i});
    }
    done = true;
  }
  EXPECT_EQ(torn.load(), 0);
}

TEST(TradeCountersTest, ConcurrentUpdatesAreExact) {
  TradeCounters c;
  {
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t) {
      ts.emplace_back([&] {
        for (int i = 0; i < 10'000; ++i) {
          c.RecordFill(3);
          if (i % 10 == 0) c.RecordReject();
        }
      });
    }
  }
  EXPECT_EQ(c.fills(), 40'000);
  EXPECT_EQ(c.volume(), 120'000);
  EXPECT_EQ(c.rejects(), 4'000);
}

TEST(HistogramTest, ParallelMatchesSerial) {
  std::vector<double> pnl(100'001);
  std::mt19937_64 rng(2);
  std::normal_distribution<double> n(0.0, 1.0);
  for (auto& x : pnl) x = n(rng);
  pnl[0] = -100.0;  // clamps to first bin
  pnl[1] = 100.0;   // clamps to last bin
  const auto serial = PnlHistogram(pnl, -4.0, 4.0, 64);
  for (unsigned t : {1u, 3u, 8u}) {
    EXPECT_EQ(ParallelPnlHistogram(pnl, -4.0, 4.0, 64, t), serial);
  }
  long long total = 0;
  for (auto b : serial) total += b;
  EXPECT_EQ(total, 100'001);
  EXPECT_TRUE(ParallelPnlHistogram(pnl, 0, 1, 0, 2).empty());
}

TEST(PublishOnceTest, ConsumerSeesPayload) {
  PublishOnce<std::vector<double>> curve;
  double seen = 0.0;
  std::jthread consumer([&] { seen = curve.Wait().at(2); });
  curve.Publish({0.01, 0.02, 0.03});
  consumer.join();
  EXPECT_DOUBLE_EQ(seen, 0.03);
  EXPECT_TRUE(curve.ready());
}

}  // namespace
}  // namespace cpp4q::williams::ch05
