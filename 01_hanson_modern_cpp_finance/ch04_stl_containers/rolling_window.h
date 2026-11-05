// Chapter 4: a fixed-capacity ring buffer with O(1) rolling mean/variance.
//
// Iterators: RollingWindow exposes a forward iterator (oldest -> newest) so
// it works with range-for and <algorithm>.

#ifndef CPP4Q_HANSON_CH04_STL_CONTAINERS_ROLLING_WINDOW_H_
#define CPP4Q_HANSON_CH04_STL_CONTAINERS_ROLLING_WINDOW_H_

#include <cstddef>
#include <iterator>
#include <vector>

namespace cpp4q::hanson::ch04 {

class RollingWindow {
 public:
  class ConstIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = double;
    using difference_type = std::ptrdiff_t;
    using pointer = const double*;
    using reference = const double&;

    ConstIterator() = default;
    reference operator*() const {
      return (*buf_)[(head_ + pos_) % buf_->size()];
    }
    ConstIterator& operator++() {
      ++pos_;
      return *this;
    }
    ConstIterator operator++(int) {
      ConstIterator tmp = *this;
      ++pos_;
      return tmp;
    }
    friend bool operator==(const ConstIterator& a, const ConstIterator& b) {
      return a.pos_ == b.pos_;
    }

   private:
    friend class RollingWindow;
    ConstIterator(const std::vector<double>* buf, std::size_t head,
                  std::size_t pos)
        : buf_(buf), head_(head), pos_(pos) {}
    const std::vector<double>* buf_ = nullptr;
    std::size_t head_ = 0;
    std::size_t pos_ = 0;
  };

  // Throws std::invalid_argument if capacity == 0.
  explicit RollingWindow(std::size_t capacity);

  void Push(double x);

  [[nodiscard]] std::size_t size() const { return size_; }
  [[nodiscard]] std::size_t capacity() const { return buffer_.size(); }
  [[nodiscard]] bool full() const { return size_ == buffer_.size(); }
  [[nodiscard]] double Mean() const;
  // Sample variance (n - 1). Returns 0 for fewer than two points.
  [[nodiscard]] double Variance() const;

  [[nodiscard]] ConstIterator begin() const { return {&buffer_, head_, 0}; }
  [[nodiscard]] ConstIterator end() const { return {&buffer_, head_, size_}; }

 private:
  void Resync();  // exact two-pass recompute (only called when full)

  std::vector<double> buffer_;
  std::size_t head_ = 0;  // index of the oldest element
  std::size_t size_ = 0;
  // Welford running moments: numerically stable even for prices ~1e6 with
  // tiny dispersion, where sum / sum-of-squares cancels catastrophically.
  double mean_ = 0.0;
  double m2_ = 0.0;
};

static_assert(std::forward_iterator<RollingWindow::ConstIterator>);

}  // namespace cpp4q::hanson::ch04

#endif  // CPP4Q_HANSON_CH04_STL_CONTAINERS_ROLLING_WINDOW_H_
