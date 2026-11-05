// Chapter 4: waiting for an event with a condition variable.
//
// An unbounded MPMC queue guarded by one mutex. Consumers block in
// WaitAndPop until an item arrives, the queue is closed, or (C++20) their
// stop_token is triggered. Close() is the clean shutdown path: it wakes
// every waiter, lets them drain what is left, then makes WaitAndPop return
// std::nullopt.

#ifndef CPP4Q_WILLIAMS_CH04_SYNCHRONIZING_THREAD_SAFE_QUEUE_H_
#define CPP4Q_WILLIAMS_CH04_SYNCHRONIZING_THREAD_SAFE_QUEUE_H_

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stop_token>
#include <utility>

namespace cpp4q::williams::ch04 {

template <typename T>
class ThreadSafeQueue {
 public:
  ThreadSafeQueue() = default;
  ThreadSafeQueue(const ThreadSafeQueue&) = delete;
  ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

  // Returns false if the queue is closed (the item is dropped).
  bool Push(T value) {
    bool wake;
    {
      std::lock_guard lock(mutex_);
      if (closed_) return false;
      items_.push_back(std::move(value));
      wake = waiters_ > 0;
    }
    // Notify after unlocking so the woken thread doesn't block on mutex_,
    // and only if someone is waiting: condition_variable_any::notify_one
    // takes an internal lock even when nobody waits, which cost ~2x in the
    // two-producer benchmark (profiling/REPORT.md, finding 6).
    if (wake) cv_.notify_one();
    return true;
  }

  [[nodiscard]] std::optional<T> WaitAndPop() {
    std::unique_lock lock(mutex_);
    ++waiters_;
    cv_.wait(lock, [this] { return !items_.empty() || closed_; });
    --waiters_;
    return PopLocked();
  }

  // Also returns std::nullopt once `stop` is requested.
  [[nodiscard]] std::optional<T> WaitAndPop(std::stop_token stop) {
    std::unique_lock lock(mutex_);
    ++waiters_;
    const bool ready =
        cv_.wait(lock, stop, [this] { return !items_.empty() || closed_; });
    --waiters_;
    if (!ready) return std::nullopt;
    return PopLocked();
  }

  [[nodiscard]] std::optional<T> TryPop() {
    std::lock_guard lock(mutex_);
    return PopLocked();
  }

  void Close() {
    {
      std::lock_guard lock(mutex_);
      closed_ = true;
    }
    cv_.notify_all();
  }

  [[nodiscard]] bool empty() const {
    std::lock_guard lock(mutex_);
    return items_.empty();
  }
  [[nodiscard]] std::size_t size() const {
    std::lock_guard lock(mutex_);
    return items_.size();
  }

 private:
  std::optional<T> PopLocked() {
    if (items_.empty()) return std::nullopt;
    std::optional<T> out(std::move(items_.front()));
    items_.pop_front();
    return out;
  }

  mutable std::mutex mutex_;
  // condition_variable_any so the stop_token overload of wait() exists.
  std::condition_variable_any cv_;
  std::deque<T> items_;
  bool closed_ = false;
  int waiters_ = 0;  // threads inside cv_.wait; guarded by mutex_
};

}  // namespace cpp4q::williams::ch04

#endif  // CPP4Q_WILLIAMS_CH04_SYNCHRONIZING_THREAD_SAFE_QUEUE_H_
