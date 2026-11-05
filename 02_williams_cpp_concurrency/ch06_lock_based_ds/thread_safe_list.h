// Chapter 6: a linked list with hand-over-hand locking.
//
// Every node has its own mutex. Traversal holds at most two node locks at a
// time (current and next), always in list order, so different threads can
// work on different parts of the list concurrently without deadlock.

#ifndef CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_THREAD_SAFE_LIST_H_
#define CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_THREAD_SAFE_LIST_H_

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace cpp4q::williams::ch06 {

template <typename T>
class ThreadSafeList {
 public:
  ThreadSafeList() = default;
  ThreadSafeList(const ThreadSafeList&) = delete;
  ThreadSafeList& operator=(const ThreadSafeList&) = delete;
  ~ThreadSafeList() {
    RemoveIf([](const T&) { return true; });
  }

  void PushFront(T value) {
    auto node = std::make_unique<Node>(std::move(value));
    std::lock_guard lock(head_.mutex);
    node->next = std::move(head_.next);
    head_.next = std::move(node);
  }

  template <typename F>
  void ForEach(F f) const {
    const Node* current = &head_;
    std::unique_lock lock(head_.mutex);
    while (const Node* next = current->next.get()) {
      std::unique_lock next_lock(next->mutex);
      lock.unlock();
      f(*next->value);
      current = next;
      lock = std::move(next_lock);
    }
  }

  template <typename Pred>
  [[nodiscard]] std::optional<T> FindFirstIf(Pred p) const {
    const Node* current = &head_;
    std::unique_lock lock(head_.mutex);
    while (const Node* next = current->next.get()) {
      std::unique_lock next_lock(next->mutex);
      lock.unlock();
      if (p(*next->value)) return *next->value;
      current = next;
      lock = std::move(next_lock);
    }
    return std::nullopt;
  }

  // Returns the number of elements removed.
  template <typename Pred>
  std::size_t RemoveIf(Pred p) {
    std::size_t removed = 0;
    Node* current = &head_;
    std::unique_lock lock(head_.mutex);
    while (Node* next = current->next.get()) {
      std::unique_lock next_lock(next->mutex);
      if (p(*next->value)) {
        std::unique_ptr<Node> doomed = std::move(current->next);
        current->next = std::move(next->next);
        next_lock.unlock();  // must unlock before `doomed` destroys the mutex
        ++removed;
      } else {
        lock.unlock();
        current = next;
        lock = std::move(next_lock);
      }
    }
    return removed;
  }

 private:
  struct Node {
    Node() = default;
    explicit Node(T v) : value(std::make_unique<T>(std::move(v))) {}
    mutable std::mutex mutex;
    std::unique_ptr<T> value;
    std::unique_ptr<Node> next;
  };
  Node head_;  // sentinel; never holds a value
};

}  // namespace cpp4q::williams::ch06

#endif  // CPP4Q_WILLIAMS_CH06_LOCK_BASED_DS_THREAD_SAFE_LIST_H_
