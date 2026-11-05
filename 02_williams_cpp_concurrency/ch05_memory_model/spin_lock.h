// Chapter 5: atomic_flag and acquire/release ordering.
//
// A test-and-test-and-set spin lock. lock() spins on a relaxed load (which
// stays in the local cache) and only attempts the read-modify-write when the
// lock looks free, so waiters don't bounce the cache line between cores.
// After a few spins it uses C++20 atomic wait() so a long wait parks the
// thread instead of burning a core. Meets Lockable, so lock_guard works.

#ifndef CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_SPIN_LOCK_H_
#define CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_SPIN_LOCK_H_

#include <atomic>

namespace cpp4q::williams::ch05 {

class SpinLock {
 public:
  void lock() noexcept {
    for (int round = 0;; ++round) {
      if (!locked_.exchange(true, std::memory_order_acquire)) return;
      if (round < kSpinRounds) {
        // The pause hint stops the core from flooding the memory system
        // with speculative loads and frees resources for a hyper-thread
        // sibling (the holder, perhaps). Without it the 4-thread benchmark
        // ran 2.2x slower than std::mutex; with it, 1.9x. Exponential
        // backoff didn't help further. Under real contention a futex-backed
        // std::mutex wins (profiling/REPORT.md, finding 8).
        while (locked_.load(std::memory_order_relaxed)) CpuRelax();
      } else {
        locked_.wait(true, std::memory_order_relaxed);
      }
    }
  }
  bool try_lock() noexcept {
    return !locked_.load(std::memory_order_relaxed) &&
           !locked_.exchange(true, std::memory_order_acquire);
  }
  void unlock() noexcept {
    locked_.store(false, std::memory_order_release);
    locked_.notify_one();
  }

 private:
  static constexpr int kSpinRounds = 64;
  static void CpuRelax() noexcept {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    asm volatile("yield");
#endif
  }
  std::atomic<bool> locked_{false};
};

}  // namespace cpp4q::williams::ch05

#endif  // CPP4Q_WILLIAMS_CH05_MEMORY_MODEL_SPIN_LOCK_H_
