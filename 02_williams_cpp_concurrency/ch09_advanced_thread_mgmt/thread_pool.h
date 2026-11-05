// Chapter 9: Advanced thread management - a work-stealing thread pool.
//
// * Submit() wraps any callable in a packaged_task and returns its future.
// * Tasks submitted from a pool thread go to that thread's own deque (LIFO
//   for cache locality); tasks from outside go to a shared queue.
// * An idle worker first pops its own deque, then the shared queue, then
//   steals from the back (oldest end) of another worker's deque.
// * WaitFor(future) runs pending tasks while it waits, so a task that
//   submits sub-tasks and waits for them (recursive divide and conquer)
//   can't deadlock the pool.
// * Workers are std::jthreads; the destructor requests stop and joins.

#ifndef CPP4Q_WILLIAMS_CH09_ADVANCED_THREAD_MGMT_THREAD_POOL_H_
#define CPP4Q_WILLIAMS_CH09_ADVANCED_THREAD_MGMT_THREAD_POOL_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace cpp4q::williams::ch09 {

// Move-only type-erased void() callable (std::move_only_function is C++23).
class Task {
 public:
  Task() = default;
  template <typename F>
    requires(!std::is_same_v<std::decay_t<F>, Task> &&
             std::is_invocable_v<std::decay_t<F>&>)
  explicit Task(F&& f)
      : impl_(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(f))) {}
  void operator()() { impl_->Call(); }
  explicit operator bool() const { return impl_ != nullptr; }

 private:
  struct Concept {
    virtual ~Concept() = default;
    virtual void Call() = 0;
  };
  template <typename F>
  struct Model final : Concept {
    explicit Model(F&& f) : fn(std::move(f)) {}
    explicit Model(const F& f) : fn(f) {}
    void Call() override { fn(); }
    F fn;
  };
  std::unique_ptr<Concept> impl_;
};

class ThreadPool {
 public:
  explicit ThreadPool(unsigned num_threads = 0);
  ~ThreadPool();
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  template <typename F>
  [[nodiscard]] std::future<std::invoke_result_t<std::decay_t<F>&>> Submit(
      F&& f) {
    using R = std::invoke_result_t<std::decay_t<F>&>;
    std::packaged_task<R()> task(std::forward<F>(f));
    auto result = task.get_future();
    Enqueue(Task(std::move(task)));
    return result;
  }

  // Runs one queued task on the calling thread if any is available.
  bool RunPendingTask();

  // Blocks until `f` is ready, running other tasks meanwhile.
  template <typename R>
  R WaitFor(std::future<R>& f) {
    while (f.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
      if (!RunPendingTask()) std::this_thread::yield();
    }
    return f.get();
  }

  [[nodiscard]] unsigned size() const {
    return static_cast<unsigned>(workers_.size());
  }

 private:
  struct WorkQueue {
    std::mutex mutex;
    std::deque<Task> tasks;
    // Mirrors tasks.size(); read without the lock so idle workers can skip
    // empty queues instead of convoying on their mutexes.
    std::atomic<std::size_t> size{0};
  };

  void Enqueue(Task task);
  void WorkerLoop(std::stop_token stop, unsigned index);
  std::optional<Task> PopLocal();
  std::optional<Task> PopShared();
  std::optional<Task> Steal();

  std::vector<std::unique_ptr<WorkQueue>> queues_;  // one per worker
  WorkQueue shared_;
  std::mutex sleep_mutex_;
  std::condition_variable_any wake_;
  std::atomic<std::int64_t> pending_{0};  // queued, not yet taken (signed)
  std::atomic<int> sleepers_{0};          // workers inside wake_.wait()
  std::vector<std::jthread> workers_;

  // Which pool (if any) the current thread works for, and its queue index.
  static thread_local ThreadPool* tls_pool_;
  static thread_local unsigned tls_index_;
};

// Parallel quicksort on the pool: each partition step submits the lower
// half as a task and recurses on the upper half, then helps the pool while
// waiting for the lower half.
template <typename T>
void PoolQuickSort(ThreadPool& pool, std::vector<T>& v,
                   std::size_t serial_cutoff = 8192);

// A background thread that reprices a position on every new spot until it
// is stopped. Shows std::jthread + std::stop_token cooperative
// cancellation with condition_variable_any (C++20's replacement for the
// book's hand-rolled interruptible_thread).
class BackgroundRepricer {
 public:
  explicit BackgroundRepricer(double quantity);
  ~BackgroundRepricer();  // requests stop and joins
  void OnSpot(double spot);
  // Blocks until the value reflecting update number `version` is computed.
  double WaitForValue(std::uint64_t version);
  void Stop() { thread_.request_stop(); }
  [[nodiscard]] bool stopped() const;

 private:
  void Run(std::stop_token stop);

  const double quantity_;
  mutable std::mutex mutex_;
  std::condition_variable_any cv_;
  double spot_ = 0.0;
  std::uint64_t spot_version_ = 0;
  double value_ = 0.0;
  std::uint64_t value_version_ = 0;
  bool exited_ = false;
  std::jthread thread_;  // last: started after the members above exist
};

}  // namespace cpp4q::williams::ch09

#include "ch09_advanced_thread_mgmt/thread_pool_inl.h"

#endif  // CPP4Q_WILLIAMS_CH09_ADVANCED_THREAD_MGMT_THREAD_POOL_H_
