// Chapter 6: a queue with finer-grained locking.
//
// A singly linked list with a dummy tail node. Push() only touches the
// tail and Pop() only touches the head, so each end has its own mutex and
// a producer and a consumer never contend unless the queue is empty. The
// dummy node is what keeps head and tail from ever being the same "real"
// node, so the two locks never protect the same data.

#ifndef CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_FINE_GRAINED_QUEUE_H_
#define CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_FINE_GRAINED_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace cpp4q::williams::ch06 {

template <typename T>
class FineGrainedQueue {
 public:
  FineGrainedQueue() : head_(std::make_unique<Node>()), tail_(head_.get()) {}
  FineGrainedQueue(const FineGrainedQueue&) = delete;
  FineGrainedQueue& operator=(const FineGrainedQueue&) = delete;
  ~FineGrainedQueue() {
    // Iterative teardown: the default recursive unique_ptr chain would
    // overflow the stack for a long queue.
    while (head_) head_ = std::move(head_->next);
  }

  void Push(T value) {
    auto new_dummy = std::make_unique<Node>();
    {
      std::lock_guard tail_lock(tail_mutex_);
      tail_->value.emplace(std::move(value));
      Node* const new_tail = new_dummy.get();
      tail_->next = std::move(new_dummy);
      tail_ = new_tail;
    }
    // A waiter checks its predicate under head_mutex_, not tail_mutex_. A
    // notify sent without going through head_mutex_ could land between
    // that check and the wait and be lost (the book's version has this
    // lost-wakeup window). So: notify while holding head_mutex_, but only
    // when a consumer is actually waiting, or every push would contend
    // with the consumer for the head lock (profiling/REPORT.md, finding 7).
    //
    // Dekker-style handshake: a waiter bumps waiters_ (seq_cst) before it
    // reads the tail under tail_mutex_; we read waiters_ after releasing
    // tail_mutex_. Either it sees our new node or we see it waiting.
    if (waiters_.load(std::memory_order_seq_cst) > 0) {
      std::lock_guard head_lock(head_mutex_);
      cv_.notify_one();
    }
  }

  [[nodiscard]] std::optional<T> TryPop() {
    std::lock_guard head_lock(head_mutex_);
    if (head_.get() == Tail()) return std::nullopt;
    return PopHeadLocked();
  }

  [[nodiscard]] T WaitAndPop() {
    std::unique_lock head_lock(head_mutex_);
    if (head_.get() == Tail()) {
      waiters_.fetch_add(1, std::memory_order_seq_cst);
      cv_.wait(head_lock, [this] { return head_.get() != Tail(); });
      waiters_.fetch_sub(1, std::memory_order_relaxed);
    }
    return *PopHeadLocked();
  }

  [[nodiscard]] bool empty() const {
    std::lock_guard head_lock(head_mutex_);
    return head_.get() == Tail();
  }

 private:
  struct Node {
    std::optional<T> value;
    std::unique_ptr<Node> next;
  };

  const Node* Tail() const {
    std::lock_guard tail_lock(tail_mutex_);
    return tail_;
  }

  // Requires head_mutex_ held and the queue non-empty.
  std::optional<T> PopHeadLocked() {
    std::unique_ptr<Node> old_head = std::move(head_);
    head_ = std::move(old_head->next);
    return std::move(old_head->value);
  }

  mutable std::mutex head_mutex_;
  std::unique_ptr<Node> head_;
  mutable std::mutex tail_mutex_;
  Node* tail_;
  std::condition_variable cv_;
  std::atomic<int> waiters_{0};
};

}  // namespace cpp4q::williams::ch06

#endif  // CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_FINE_GRAINED_QUEUE_H_
