// Chapter 3: Sharing data between threads - protecting invariants with
// mutexes and avoiding deadlock.
//
// Transfer() must lock two accounts. Locking them one after the other in
// argument order deadlocks when two threads transfer A->B and B->A at the
// same time. Two standard cures: std::scoped_lock(a, b), which uses a
// try-and-back-off algorithm, or a global lock order. Transfer() uses the
// second (order by address); see account.cc for why.

#ifndef CPP4Q_WILLIAMS_CH03_SHARING_DATA_ACCOUNT_H_
#define CPP4Q_WILLIAMS_CH03_SHARING_DATA_ACCOUNT_H_

#include <cstdint>
#include <mutex>

namespace cpp4q::williams::ch03 {

// Cash balance in integer cents so totals are exact.
class Account {
 public:
  explicit Account(std::int64_t opening_cents = 0) : balance_(opening_cents) {}
  Account(const Account&) = delete;
  Account& operator=(const Account&) = delete;

  [[nodiscard]] std::int64_t Balance() const {
    std::lock_guard lock(mutex_);
    return balance_;
  }
  void Deposit(std::int64_t cents) {
    std::lock_guard lock(mutex_);
    balance_ += cents;
  }

  // Moves `cents` from `from` to `to` atomically with respect to both
  // accounts. Returns false (and changes nothing) on insufficient funds or
  // when from and to are the same account.
  friend bool Transfer(Account& from, Account& to, std::int64_t cents);

 private:
  mutable std::mutex mutex_;
  std::int64_t balance_;
};

// Namespace-scope declaration so ch03::Transfer(...) also works.
bool Transfer(Account& from, Account& to, std::int64_t cents);

}  // namespace cpp4q::williams::ch03

#endif  // CPP4Q_WILLIAMS_CH03_SHARING_DATA_ACCOUNT_H_
