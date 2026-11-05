#include "ch03_sharing_data/hierarchical_mutex.h"

#include <stdexcept>

namespace cpp4q::williams::ch03 {

thread_local unsigned long HierarchicalMutex::this_thread_level_ = ULONG_MAX;

void HierarchicalMutex::CheckForViolation() const {
  if (this_thread_level_ <= level_) {
    throw std::logic_error("HierarchicalMutex: lock hierarchy violated");
  }
}

void HierarchicalMutex::UpdateLevel() {
  previous_level_ = this_thread_level_;
  this_thread_level_ = level_;
}

void HierarchicalMutex::lock() {
  CheckForViolation();
  mutex_.lock();
  UpdateLevel();
}

void HierarchicalMutex::unlock() {
  if (this_thread_level_ != level_) {
    throw std::logic_error("HierarchicalMutex: unlock out of order");
  }
  this_thread_level_ = previous_level_;
  mutex_.unlock();
}

bool HierarchicalMutex::try_lock() {
  CheckForViolation();
  if (!mutex_.try_lock()) return false;
  UpdateLevel();
  return true;
}

unsigned long HierarchicalMutex::CurrentThreadLevel() {
  return this_thread_level_;
}

}  // namespace cpp4q::williams::ch03
