#include "ch04_stl_containers/rolling_window.h"

#include <algorithm>
#include <stdexcept>

namespace cpp4q::hanson::ch04 {

RollingWindow::RollingWindow(std::size_t capacity) : buffer_(capacity) {
  if (capacity == 0) {
    throw std::invalid_argument("RollingWindow: capacity must be > 0");
  }
}

void RollingWindow::Push(double x) {
  if (full()) {
    // Replace the oldest point: update mean and M2 in one step.
    const double old = buffer_[head_];
    buffer_[head_] = x;
    head_ = (head_ + 1) % buffer_.size();
    const double old_mean = mean_;
    mean_ += (x - old) / static_cast<double>(size_);
    m2_ += (x - old) * (x - mean_ + old - old_mean);
    // Rolling updates accumulate rounding error; once per full turn of the
    // ring, recompute exactly. Amortised cost stays O(1) per Push.
    if (head_ == 0) Resync();
  } else {
    buffer_[(head_ + size_) % buffer_.size()] = x;
    ++size_;
    const double delta = x - mean_;
    mean_ += delta / static_cast<double>(size_);
    m2_ += delta * (x - mean_);
  }
}

void RollingWindow::Resync() {
  double mean = 0.0;
  for (const double v : buffer_) mean += v;
  mean /= static_cast<double>(size_);
  double m2 = 0.0;
  for (const double v : buffer_) m2 += (v - mean) * (v - mean);
  mean_ = mean;
  m2_ = m2;
}

double RollingWindow::Mean() const { return mean_; }

double RollingWindow::Variance() const {
  if (size_ < 2) return 0.0;
  return std::max(0.0, m2_ / static_cast<double>(size_ - 1));
}

}  // namespace cpp4q::hanson::ch04
