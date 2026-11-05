// Chapter 2: Managing threads.
//
// RAII owners for std::thread. ScopedThread takes ownership of an already
// running thread and joins it on scope exit; JoiningThread is a minimal,
// move-only std::jthread without stop tokens. Both make it impossible to
// destroy a joinable std::thread (which calls std::terminate).

#ifndef CPP4Q_WILLIAMS_CH02_MANAGING_THREADS_THREAD_GUARDS_H_
#define CPP4Q_WILLIAMS_CH02_MANAGING_THREADS_THREAD_GUARDS_H_

#include <stdexcept>
#include <thread>
#include <utility>

namespace cpp4q::williams::ch02 {

class ScopedThread {
 public:
  explicit ScopedThread(std::thread t) : thread_(std::move(t)) {
    if (!thread_.joinable()) {
      throw std::logic_error("ScopedThread: thread is not joinable");
    }
  }
  ~ScopedThread() { thread_.join(); }

  ScopedThread(const ScopedThread&) = delete;
  ScopedThread& operator=(const ScopedThread&) = delete;

 private:
  std::thread thread_;
};

class JoiningThread {
 public:
  JoiningThread() noexcept = default;

  template <typename F, typename... Args>
    requires(!std::is_same_v<std::remove_cvref_t<F>, JoiningThread> &&
             !std::is_same_v<std::remove_cvref_t<F>, std::thread>)
  explicit JoiningThread(F&& f, Args&&... args)
      : thread_(std::forward<F>(f), std::forward<Args>(args)...) {}

  explicit JoiningThread(std::thread t) noexcept : thread_(std::move(t)) {}

  JoiningThread(JoiningThread&& other) noexcept = default;
  JoiningThread& operator=(JoiningThread&& other) noexcept {
    if (this != &other) {
      Join();
      thread_ = std::move(other.thread_);
    }
    return *this;
  }
  JoiningThread(const JoiningThread&) = delete;
  JoiningThread& operator=(const JoiningThread&) = delete;

  ~JoiningThread() { Join(); }

  [[nodiscard]] bool joinable() const noexcept { return thread_.joinable(); }
  [[nodiscard]] std::thread::id get_id() const noexcept {
    return thread_.get_id();
  }
  void Join() {
    if (thread_.joinable()) thread_.join();
  }

 private:
  std::thread thread_;
};

}  // namespace cpp4q::williams::ch02

#endif  // CPP4Q_WILLIAMS_CH02_MANAGING_THREADS_THREAD_GUARDS_H_
