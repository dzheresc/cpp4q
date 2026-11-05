// Appendix C: a message-passing framework.
//
// The book builds a type-erased message queue plus chained dispatcher
// templates. With C++17/20 the same idea is shorter: a mailbox holds a
// std::variant of the message types an actor understands, and the actor
// handles a message by std::visit with an overload set. The compiler then
// checks that every message type has a handler.

#ifndef CPP4Q_WILLIAMS_APPENDICES_MAILBOX_H_
#define CPP4Q_WILLIAMS_APPENDICES_MAILBOX_H_

#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>
#include <variant>

namespace cpp4q::williams::appendix_c {

template <typename... Fs>
struct Overloaded : Fs... {
  using Fs::operator()...;
};
template <typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

template <typename... Msgs>
class Mailbox {
 public:
  using Message = std::variant<Msgs...>;

  template <typename M>
  void Send(M&& message) {
    // Notify while holding the lock. Notifying after unlock is also correct
    // and is the usual advice, but helgrind and drd flag every such signal
    // as "dubious" (14k reports on the gateway workload). BM_GatewayOrderFlow
    // measured 24.8 ms in-lock vs 23.5 ms after-unlock (median of 5), which
    // is inside this machine's run-to-run noise.
    std::lock_guard lock(mutex_);
    queue_.emplace_back(std::forward<M>(message));
    cv_.notify_one();
  }

  [[nodiscard]] Message Receive() {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return !queue_.empty(); });
    Message m = std::move(queue_.front());
    queue_.pop_front();
    return m;
  }

  // Waits for one message and dispatches it to the matching handler.
  template <typename Visitor>
  decltype(auto) ReceiveAndHandle(Visitor&& visitor) {
    return std::visit(std::forward<Visitor>(visitor), Receive());
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<Message> queue_;
};

}  // namespace cpp4q::williams::appendix_c

#endif  // CPP4Q_WILLIAMS_APPENDICES_MAILBOX_H_
