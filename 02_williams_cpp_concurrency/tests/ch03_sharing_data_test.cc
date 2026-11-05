#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "ch03_sharing_data/account.h"
#include "ch03_sharing_data/hierarchical_mutex.h"
#include "ch03_sharing_data/quote_cache.h"

namespace cpp4q::williams::ch03 {
namespace {

TEST(AccountTest, TransferMovesMoney) {
  Account a(100), b(0);
  EXPECT_TRUE(Transfer(a, b, 40));
  EXPECT_EQ(a.Balance(), 60);
  EXPECT_EQ(b.Balance(), 40);
  EXPECT_FALSE(Transfer(a, b, 61));  // insufficient funds
  EXPECT_FALSE(Transfer(a, a, 1));   // self transfer refused
  EXPECT_FALSE(Transfer(a, b, -5));
  EXPECT_EQ(a.Balance() + b.Balance(), 100);
}

TEST(AccountTest, CrossingTransfersDontDeadlockAndConserveMoney) {
  constexpr int kAccounts = 5;
  std::vector<std::unique_ptr<Account>> accounts;
  for (int i = 0; i < kAccounts; ++i) {
    accounts.push_back(std::make_unique<Account>(10'000));
  }
  {
    std::vector<std::jthread> threads;
    for (int t = 0; t < 4; ++t) {
      threads.emplace_back([&, t] {
        for (int i = 0; i < 20'000; ++i) {
          const auto from = static_cast<std::size_t>((i + t) % kAccounts);
          const auto to = static_cast<std::size_t>((i * 3 + t + 1) % kAccounts);
          Transfer(*accounts[from], *accounts[to], 1 + i % 7);
        }
      });
    }
  }
  std::int64_t total = 0;
  for (const auto& a : accounts) {
    EXPECT_GE(a->Balance(), 0);
    total += a->Balance();
  }
  EXPECT_EQ(total, kAccounts * 10'000);
}

TEST(HierarchicalMutexTest, DescendingOrderIsAllowed) {
  HierarchicalMutex high(100), low(50);
  std::lock_guard a(high);
  EXPECT_EQ(HierarchicalMutex::CurrentThreadLevel(), 100u);
  {
    std::lock_guard b(low);
    EXPECT_EQ(HierarchicalMutex::CurrentThreadLevel(), 50u);
  }
  EXPECT_EQ(HierarchicalMutex::CurrentThreadLevel(), 100u);
}

TEST(HierarchicalMutexTest, AscendingOrderThrows) {
  HierarchicalMutex high(100), low(50);
  std::lock_guard a(low);
  EXPECT_THROW(high.lock(), std::logic_error);
  EXPECT_THROW((void)high.try_lock(), std::logic_error);
  HierarchicalMutex same(50);
  EXPECT_THROW(same.lock(), std::logic_error);  // equal level not allowed
}

TEST(HierarchicalMutexTest, LevelIsPerThread) {
  HierarchicalMutex low(50), high(100);
  std::lock_guard a(low);
  bool ok = false;
  std::thread([&] {
    std::lock_guard b(high);  // other thread holds nothing
    ok = true;
  }).join();
  EXPECT_TRUE(ok);
}

TEST(QuoteCacheTest, UpdateAndGet) {
  QuoteCache cache;
  EXPECT_FALSE(cache.Get("ES").has_value());
  cache.Update("ES", {5000.0, 5000.25});
  cache.Update("ES", {5001.0, 5001.25});
  ASSERT_TRUE(cache.Get("ES").has_value());
  EXPECT_DOUBLE_EQ(cache.Get("ES")->Mid(), 5001.125);
  EXPECT_EQ(cache.size(), 1u);
}

TEST(QuoteCacheTest, ConcurrentReadersSeeConsistentQuotes) {
  QuoteCache cache;
  cache.Update("X", {1.0, 2.0});
  std::atomic<bool> bad{false};
  {
    std::jthread writer([&] {
      for (int i = 0; i < 20'000; ++i) {
        const double b = i;
        cache.Update("X", {b, b + 1.0});
      }
    });
    std::vector<std::jthread> readers;
    for (int r = 0; r < 3; ++r) {
      readers.emplace_back([&] {
        for (int i = 0; i < 20'000; ++i) {
          const auto q = cache.Get("X");
          if (!q || q->ask - q->bid != 1.0) bad = true;
        }
      });
    }
  }
  EXPECT_FALSE(bad.load());
}

TEST(LazyCurveTest, BuildsExactlyOnceUnderContention) {
  std::atomic<int> builds{0};
  LazyCurve curve([&] {
    ++builds;
    return ZeroCurve{{1.0, 2.0}, {0.02, 0.04}};
  });
  {
    std::vector<std::jthread> readers;
    for (int i = 0; i < 8; ++i) {
      readers.emplace_back([&] { (void)curve.Get().Discount(1.5); });
    }
  }
  EXPECT_EQ(builds.load(), 1);
  EXPECT_NEAR(curve.Get().Discount(1.5), std::exp(-0.03 * 1.5), 1e-15);
  EXPECT_NEAR(curve.Get().Discount(0.5), std::exp(-0.02 * 0.5), 1e-15);
  EXPECT_NEAR(curve.Get().Discount(3.0), std::exp(-0.04 * 3.0), 1e-15);
}

TEST(LazyCurveTest, RetriesAfterBuilderThrows) {
  int attempts = 0;
  LazyCurve curve([&] {
    if (++attempts == 1) throw std::runtime_error("feed down");
    return ZeroCurve{{1.0}, {0.01}};
  });
  EXPECT_THROW((void)curve.Get(), std::runtime_error);
  EXPECT_NO_THROW((void)curve.Get());
  EXPECT_EQ(attempts, 2);
}

}  // namespace
}  // namespace cpp4q::williams::ch03
