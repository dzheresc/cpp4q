// Chapter 3: protecting rarely-updated data with std::shared_mutex, and
// thread-safe one-time initialisation.
//
// QuoteCache: many pricing threads read, one feed handler writes. Readers
// take a shared lock and run in parallel; the writer takes it exclusively.
// LazyCurve: an expensive reference object built on first use by exactly
// one thread, however many threads ask for it at once.

#ifndef CPP4Q_WILLIAMS_CH03_SHARING_DATA_QUOTE_CACHE_H_
#define CPP4Q_WILLIAMS_CH03_SHARING_DATA_QUOTE_CACHE_H_

#include <atomic>
#include <cstddef>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace cpp4q::williams::ch03 {

struct Quote {
  double bid = 0.0;
  double ask = 0.0;
  [[nodiscard]] double Mid() const { return 0.5 * (bid + ask); }
  friend bool operator==(const Quote&, const Quote&) = default;
};

class QuoteCache {
 public:
  void Update(std::string_view symbol, const Quote& quote);
  // Returns a copy: handing out a reference would let the caller read it
  // after the lock is released.
  [[nodiscard]] std::optional<Quote> Get(std::string_view symbol) const;
  [[nodiscard]] std::size_t size() const;

 private:
  mutable std::shared_mutex mutex_;
  std::map<std::string, Quote, std::less<>> quotes_;
};

// Discount curve on a fixed tenor grid; built once, then read-only.
struct ZeroCurve {
  std::vector<double> tenors;
  std::vector<double> zero_rates;
  // Linear interpolation on zero rates, flat extrapolation.
  [[nodiscard]] double Discount(double t) const;
};

// Double-checked locking done right: an acquire load on the fast path,
// and the build under a mutex with a release store. The first version used
// std::call_once, the textbook tool, but libstdc++ implements it with
// pthread_once, and a builder that throws then hangs the next caller under
// ThreadSanitizer (and on some targets natively, GCC bug 66146). A curve
// builder that fails because a market-data feed is down is not exotic, so
// LazyCurve guarantees retry-after-throw itself.
class LazyCurve {
 public:
  explicit LazyCurve(std::function<ZeroCurve()> builder)
      : builder_(std::move(builder)) {}

  // Thread-safe; the builder runs once. If it throws, the exception
  // propagates and the next caller retries.
  [[nodiscard]] const ZeroCurve& Get() const {
    if (!ready_.load(std::memory_order_acquire)) {
      std::lock_guard lock(mutex_);
      if (!ready_.load(std::memory_order_relaxed)) {
        curve_ = builder_();
        ready_.store(true, std::memory_order_release);
      }
    }
    return curve_;
  }

 private:
  std::function<ZeroCurve()> builder_;
  mutable std::mutex mutex_;
  mutable std::atomic<bool> ready_{false};
  mutable ZeroCurve curve_;
};

}  // namespace cpp4q::williams::ch03

#endif  // CPP4Q_WILLIAMS_CH03_SHARING_DATA_QUOTE_CACHE_H_
