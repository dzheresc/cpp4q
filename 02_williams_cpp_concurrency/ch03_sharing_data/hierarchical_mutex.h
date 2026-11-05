// Chapter 3: a lock hierarchy checked at run time.
//
// Every HierarchicalMutex has a level. A thread may only lock a mutex whose
// level is strictly lower than the lowest level it already holds, so a
// cycle (and hence a deadlock) can't form. Violations throw
// std::logic_error at the lock() call instead of hanging later.

#ifndef CPP4Q_WILLIAMS_CH03_SHARING_DATA_HIERARCHICAL_MUTEX_H_
#define CPP4Q_WILLIAMS_CH03_SHARING_DATA_HIERARCHICAL_MUTEX_H_

#include <climits>
#include <mutex>

namespace cpp4q::williams::ch03 {

class HierarchicalMutex {
 public:
  explicit HierarchicalMutex(unsigned long level) : level_(level) {}
  HierarchicalMutex(const HierarchicalMutex&) = delete;
  HierarchicalMutex& operator=(const HierarchicalMutex&) = delete;

  void lock();
  void unlock();
  bool try_lock();

  [[nodiscard]] unsigned long level() const { return level_; }
  // The level the calling thread currently holds (ULONG_MAX if none).
  [[nodiscard]] static unsigned long CurrentThreadLevel();

 private:
  void CheckForViolation() const;
  void UpdateLevel();

  std::mutex mutex_;
  const unsigned long level_;
  unsigned long previous_level_ = 0;  // guarded by mutex_
  static thread_local unsigned long this_thread_level_;
};

}  // namespace cpp4q::williams::ch03

#endif  // CPP4Q_WILLIAMS_CH03_SHARING_DATA_HIERARCHICAL_MUTEX_H_
