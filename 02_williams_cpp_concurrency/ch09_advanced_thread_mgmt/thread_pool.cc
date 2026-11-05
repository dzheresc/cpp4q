#include "ch09_advanced_thread_mgmt/thread_pool.h"

namespace cpp4q::williams::ch09 {
namespace {

inline void CpuRelax() {
#if defined(__x86_64__) || defined(__i386__)
  __builtin_ia32_pause();
#elif defined(__aarch64__)
  asm volatile("yield");
#endif
}

// How long an idle worker polls before parking on the condition variable.
// Parking and waking cost a futex syscall each; for a burst of small tasks
// that dominated the run time (see profiling/REPORT.md).
constexpr int kSpinsBeforeSleep = 4096;

}  // namespace

thread_local ThreadPool* ThreadPool::tls_pool_ = nullptr;
thread_local unsigned ThreadPool::tls_index_ = 0;

ThreadPool::ThreadPool(unsigned num_threads) {
  if (num_threads == 0) num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0) num_threads = 2;
  queues_.reserve(num_threads);
  for (unsigned i = 0; i < num_threads; ++i) {
    queues_.push_back(std::make_unique<WorkQueue>());
  }
  workers_.reserve(num_threads);
  for (unsigned i = 0; i < num_threads; ++i) {
    workers_.emplace_back(
        [this, i](std::stop_token stop) { WorkerLoop(std::move(stop), i); });
  }
}

ThreadPool::~ThreadPool() {
  for (auto& w : workers_) w.request_stop();
  wake_.notify_all();
  workers_.clear();  // joins
}

void ThreadPool::Enqueue(Task task) {
  WorkQueue& q = (tls_pool_ == this) ? *queues_[tls_index_] : shared_;
  {
    std::lock_guard lock(q.mutex);
    q.tasks.push_back(std::move(task));
    q.size.store(q.tasks.size(), std::memory_order_release);
  }
  // Count after the push, so a worker that sees pending_ > 0 will find the
  // task. (A thief can take it before this increment, so pending_ is signed
  // and may dip below zero for a moment.)
  pending_.fetch_add(1, std::memory_order_seq_cst);
  // Only touch sleep_mutex_ and the condition variable when a worker is
  // actually asleep. This is a Dekker-style handshake: the worker bumps
  // sleepers_ and then reads pending_; we bump pending_ and then read
  // sleepers_. With seq_cst at least one side sees the other, so either
  // the worker doesn't sleep or we wake it. Passing through sleep_mutex_
  // makes sure that worker is already inside wait() when we notify.
  if (sleepers_.load(std::memory_order_seq_cst) > 0) {
    {
      std::lock_guard lock(sleep_mutex_);
    }
    wake_.notify_one();
  }
}

std::optional<Task> ThreadPool::PopLocal() {
  if (tls_pool_ != this) return std::nullopt;
  WorkQueue& q = *queues_[tls_index_];
  if (q.size.load(std::memory_order_acquire) == 0) return std::nullopt;
  std::lock_guard lock(q.mutex);
  if (q.tasks.empty()) return std::nullopt;
  Task t = std::move(q.tasks.back());  // LIFO: hottest data first
  q.tasks.pop_back();
  q.size.store(q.tasks.size(), std::memory_order_relaxed);
  return t;
}

std::optional<Task> ThreadPool::PopShared() {
  if (shared_.size.load(std::memory_order_acquire) == 0) return std::nullopt;
  std::lock_guard lock(shared_.mutex);
  if (shared_.tasks.empty()) return std::nullopt;
  Task t = std::move(shared_.tasks.front());
  shared_.tasks.pop_front();
  shared_.size.store(shared_.tasks.size(), std::memory_order_relaxed);
  return t;
}

std::optional<Task> ThreadPool::Steal() {
  const auto n = static_cast<unsigned>(queues_.size());
  const unsigned start = (tls_pool_ == this) ? tls_index_ + 1 : 0;
  for (unsigned k = 0; k < n; ++k) {
    const unsigned i = (start + k) % n;
    if (tls_pool_ == this && i == tls_index_) continue;
    WorkQueue& q = *queues_[i];
    if (q.size.load(std::memory_order_acquire) == 0) continue;
    std::lock_guard lock(q.mutex);
    if (!q.tasks.empty()) {
      Task t = std::move(q.tasks.front());  // oldest: likely the biggest
      q.tasks.pop_front();
      q.size.store(q.tasks.size(), std::memory_order_relaxed);
      return t;
    }
  }
  return std::nullopt;
}

bool ThreadPool::RunPendingTask() {
  std::optional<Task> t = PopLocal();
  if (!t) t = PopShared();
  if (!t) t = Steal();
  if (!t) return false;
  pending_.fetch_sub(1, std::memory_order_relaxed);
  (*t)();
  return true;
}

void ThreadPool::WorkerLoop(std::stop_token stop, unsigned index) {
  tls_pool_ = this;
  tls_index_ = index;
  while (!stop.stop_requested()) {
    if (RunPendingTask()) continue;
    for (int i = 0; i < kSpinsBeforeSleep &&
                    pending_.load(std::memory_order_relaxed) <= 0 &&
                    !stop.stop_requested();
         ++i) {
      CpuRelax();
    }
    if (pending_.load(std::memory_order_relaxed) > 0) continue;
    std::unique_lock lock(sleep_mutex_);
    sleepers_.fetch_add(1, std::memory_order_seq_cst);
    wake_.wait(lock, stop,
               [this] { return pending_.load(std::memory_order_seq_cst) > 0; });
    sleepers_.fetch_sub(1, std::memory_order_relaxed);
  }
  // Drain whatever is left so no future is abandoned.
  while (RunPendingTask()) {
  }
  tls_pool_ = nullptr;
}

BackgroundRepricer::BackgroundRepricer(double quantity)
    : quantity_(quantity),
      thread_([this](std::stop_token stop) { Run(std::move(stop)); }) {}

BackgroundRepricer::~BackgroundRepricer() { thread_.request_stop(); }

void BackgroundRepricer::OnSpot(double spot) {
  {
    std::lock_guard lock(mutex_);
    spot_ = spot;
    ++spot_version_;
  }
  cv_.notify_all();
}

double BackgroundRepricer::WaitForValue(std::uint64_t version) {
  std::unique_lock lock(mutex_);
  cv_.wait(lock, [&] { return value_version_ >= version || exited_; });
  return value_;
}

bool BackgroundRepricer::stopped() const {
  std::lock_guard lock(mutex_);
  return exited_;
}

void BackgroundRepricer::Run(std::stop_token stop) {
  std::unique_lock lock(mutex_);
  std::uint64_t seen = 0;
  while (cv_.wait(lock, stop, [&] { return spot_version_ != seen; })) {
    seen = spot_version_;
    const double spot = spot_;
    lock.unlock();
    const double value = quantity_ * spot;  // the "expensive" revaluation
    lock.lock();
    value_ = value;
    value_version_ = seen;
    cv_.notify_all();
  }
  exited_ = true;
  cv_.notify_all();
}

}  // namespace cpp4q::williams::ch09
