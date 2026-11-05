// Chapter 11: Testing and debugging multithreaded applications.
//
// RunConcurrently() is the core of a concurrency test: it starts N threads,
// holds them at a std::latch until all exist (so they really overlap
// instead of running one after another as thread start-up staggers them),
// runs the body, joins, and rethrows the first exception on the caller.
//
// PreTradeLimit is the kind of bug such tests find: a race condition with
// no data race. Every access is atomic, yet the naive check-then-act lets
// two orders that each fit under the limit both pass. The fixed version
// makes check and reserve a single CAS.

#ifndef CPP4Q_WILLIAMS_CH11_TESTING_DEBUGGING_STRESS_HARNESS_H_
#define CPP4Q_WILLIAMS_CH11_TESTING_DEBUGGING_STRESS_HARNESS_H_

#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <latch>
#include <mutex>
#include <thread>
#include <vector>

namespace cpp4q::williams::ch11 {

// Runs body(thread_index) on `num_threads` threads that start together.
void RunConcurrently(unsigned num_threads,
                     const std::function<void(unsigned)>& body);

// Repeats RunConcurrently `rounds` times, calling check() after each round;
// returns the number of rounds in which check() returned false.
int CountFailingRounds(int rounds, unsigned num_threads,
                       const std::function<void()>& setup,
                       const std::function<void(unsigned)>& body,
                       const std::function<bool()>& check);

class PreTradeLimit {
 public:
  explicit PreTradeLimit(std::int64_t limit) : limit_(limit) {}

  // Race condition: another thread can reserve between load and fetch_add.
  bool TryReserveNaive(std::int64_t qty) {
    if (used_.load() + qty > limit_) return false;
    // Widen the window so tests see the race quickly. In production the
    // window is just as real, only rarer.
    std::this_thread::yield();
    used_.fetch_add(qty);
    return true;
  }

  // Correct: the check and the update happen in one atomic step.
  bool TryReserve(std::int64_t qty) {
    std::int64_t cur = used_.load(std::memory_order_relaxed);
    do {
      if (cur + qty > limit_) return false;
    } while (!used_.compare_exchange_weak(
        cur, cur + qty, std::memory_order_acq_rel, std::memory_order_relaxed));
    return true;
  }

  void Release(std::int64_t qty) { used_.fetch_sub(qty); }
  [[nodiscard]] std::int64_t used() const { return used_.load(); }
  [[nodiscard]] std::int64_t limit() const { return limit_; }
  void Reset() { used_.store(0); }

 private:
  const std::int64_t limit_;
  std::atomic<std::int64_t> used_{0};
};

}  // namespace cpp4q::williams::ch11

#endif  // CPP4Q_WILLIAMS_CH11_TESTING_DEBUGGING_STRESS_HARNESS_H_
