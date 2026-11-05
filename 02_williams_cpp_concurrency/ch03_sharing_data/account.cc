#include "ch03_sharing_data/account.h"

#include <functional>

namespace cpp4q::williams::ch03 {

bool Transfer(Account& from, Account& to, std::int64_t cents) {
  if (&from == &to || cents < 0) return false;  // locking twice would be UB
  // Lock in address order: a global order means no deadlock, without
  // std::lock's try-and-back-off. The first version used
  // std::scoped_lock(from.mutex_, to.mutex_), which is also deadlock-free
  // but retries under contention and trips helgrind's lock-order checker
  // (profiling/REPORT.md, finding 5).
  std::mutex& first = std::less<>{}(&from, &to) ? from.mutex_ : to.mutex_;
  std::mutex& second = std::less<>{}(&from, &to) ? to.mutex_ : from.mutex_;
  std::scoped_lock lock1(first);
  std::scoped_lock lock2(second);
  if (from.balance_ < cents) return false;
  from.balance_ -= cents;
  to.balance_ += cents;
  return true;
}

}  // namespace cpp4q::williams::ch03
