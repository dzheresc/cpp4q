// Chapter 7: a lock-free (Treiber) stack with safe memory reclamation.
//
// Push and Pop CAS the head pointer. The hard part is freeing a popped
// node: another thread may have read the same head and be about to
// dereference it. Like the book's first reclamation scheme, the stack counts
// threads inside Pop(); a popped node is deleted at once only if the
// popping thread is alone, otherwise it goes on a pending list that the
// last thread out of Pop() frees. Because nodes are freed only when no
// thread can hold a pointer to them, their addresses can't be reused under
// a concurrent Pop, which also rules out ABA.
//
// Trade-off: under constant contention the pending list may keep growing
// until a quiet moment. Hazard pointers or split reference counts (also in
// the chapter) bound that, at a higher per-operation cost.

#ifndef CPP4Q_WILLIAMS_CH07_LOCK_FREE_LOCK_FREE_STACK_H_
#define CPP4Q_WILLIAMS_CH07_LOCK_FREE_LOCK_FREE_STACK_H_

#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace cpp4q::williams::ch07 {

template <typename T>
class LockFreeStack {
 public:
  LockFreeStack() = default;
  LockFreeStack(const LockFreeStack&) = delete;
  LockFreeStack& operator=(const LockFreeStack&) = delete;
  ~LockFreeStack() {
    DeleteChain(head_.load(std::memory_order_relaxed), &Node::next);
    DeleteChain(pending_.load(std::memory_order_relaxed), &Node::pending_next);
  }

  void Push(T value) {
    Node* node = new Node{std::move(value),
                          head_.load(std::memory_order_relaxed), nullptr};
    while (!head_.compare_exchange_weak(node->next, node,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
    }
  }

  std::optional<T> Pop() {
    threads_in_pop_.fetch_add(1, std::memory_order_seq_cst);
    Node* old_head = head_.load(std::memory_order_acquire);
    // Reading old_head->next is safe: threads_in_pop_ > 0 keeps old_head
    // alive, and `next` is never written after the node is published.
    while (old_head && !head_.compare_exchange_weak(
                           old_head, old_head->next, std::memory_order_acquire,
                           std::memory_order_acquire)) {
    }
    std::optional<T> result;
    if (old_head) result.emplace(std::move(old_head->value));
    TryReclaim(old_head);
    return result;
  }

  [[nodiscard]] bool empty() const {
    return head_.load(std::memory_order_acquire) == nullptr;
  }
  // Number of popped nodes still awaiting deletion (for tests/diagnostics).
  [[nodiscard]] std::size_t pending_count() const {
    std::size_t n = 0;
    for (Node* p = pending_.load(std::memory_order_acquire); p;
         p = p->pending_next) {
      ++n;
    }
    return n;
  }

 private:
  struct Node {
    T value;
    Node* next;
    Node* pending_next;  // separate link: `next` may still be read by others
  };

  static void DeleteChain(Node* n, Node* Node::*link) {
    while (n) {
      Node* following = n->*link;
      delete n;
      n = following;
    }
  }

  void ChainPending(Node* first, Node* last) {
    last->pending_next = pending_.load(std::memory_order_relaxed);
    while (!pending_.compare_exchange_weak(last->pending_next, first,
                                           std::memory_order_release,
                                           std::memory_order_relaxed)) {
    }
  }

  void TryReclaim(Node* old_head) {
    if (threads_in_pop_.load(std::memory_order_seq_cst) == 1) {
      // Claim the pending list first, then check we are still alone: a
      // thread that entered Pop() after the claim can't see those nodes.
      Node* to_delete = pending_.exchange(nullptr, std::memory_order_acq_rel);
      if (threads_in_pop_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        DeleteChain(to_delete, &Node::pending_next);
      } else if (to_delete) {
        Node* last = to_delete;
        while (last->pending_next) last = last->pending_next;
        ChainPending(to_delete, last);
      }
      delete old_head;  // we were alone when we unlinked it
    } else {
      if (old_head) ChainPending(old_head, old_head);
      threads_in_pop_.fetch_sub(1, std::memory_order_seq_cst);
    }
  }

  std::atomic<Node*> head_{nullptr};
  std::atomic<std::size_t> threads_in_pop_{0};
  std::atomic<Node*> pending_{nullptr};
};

}  // namespace cpp4q::williams::ch07

#endif  // CPP4Q_WILLIAMS_CH07_LOCK_FREE_LOCK_FREE_STACK_H_
