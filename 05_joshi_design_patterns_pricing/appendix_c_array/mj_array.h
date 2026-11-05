// Appendix C: A simple array class.
//
// The book's MJArray: a resizable array of doubles with element-wise
// arithmetic, used for the spot paths of the exotics engine. This version
// keeps the design (size separate from capacity, so shrinking and
// re-growing within the capacity never reallocates; element-wise and
// scalar compound assignment; Sum/Min/Max) but owns its buffer through
// std::unique_ptr<double[]>, so the copy operations are the only special
// members written by hand, and moves are the defaulted noexcept ones.
// It also exposes std::span views, so it interoperates with the standard
// library.

#ifndef CPP4Q_JOSHI_APPENDIX_C_ARRAY_MJ_ARRAY_H_
#define CPP4Q_JOSHI_APPENDIX_C_ARRAY_MJ_ARRAY_H_

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>

namespace cpp4q::joshi {

class MjArray {
 public:
  MjArray() = default;
  // `size` elements, all equal to `value`.
  explicit MjArray(std::size_t size, double value = 0.0);
  MjArray(std::initializer_list<double> values);

  MjArray(const MjArray& other);
  MjArray& operator=(const MjArray& other);
  MjArray(MjArray&& other) noexcept = default;
  MjArray& operator=(MjArray&& other) noexcept = default;
  ~MjArray() = default;

  // Assigns `value` to every element.
  MjArray& operator=(double value);

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return capacity_; }
  bool empty() const { return size_ == 0; }
  // Changes the size; reallocates (without preserving the contents) only if
  // the new size exceeds the capacity.
  void resize(std::size_t new_size);

  double& operator[](std::size_t i) {
    assert(i < size_);
    return data_[i];
  }
  double operator[](std::size_t i) const {
    assert(i < size_);
    return data_[i];
  }

  double* data() { return data_.get(); }
  const double* data() const { return data_.get(); }
  double* begin() { return data_.get(); }
  double* end() { return data_.get() + size_; }
  const double* begin() const { return data_.get(); }
  const double* end() const { return data_.get() + size_; }
  std::span<double> span() { return {data_.get(), size_}; }
  std::span<const double> span() const { return {data_.get(), size_}; }

  // Element-wise; the sizes must match.
  MjArray& operator+=(const MjArray& rhs);
  MjArray& operator-=(const MjArray& rhs);
  MjArray& operator*=(const MjArray& rhs);
  MjArray& operator/=(const MjArray& rhs);

  MjArray& operator+=(double rhs);
  MjArray& operator-=(double rhs);
  MjArray& operator*=(double rhs);
  MjArray& operator/=(double rhs);

  // Throw std::logic_error on an empty array for Min and Max.
  double Sum() const;
  double Min() const;
  double Max() const;

  // *this += a * x without the temporary that `y += a * x` creates (the
  // problem expression templates solve in general).
  MjArray& AddScaled(double a, const MjArray& x);

  // Replaces each element x by f(x).
  template <class F>
  MjArray& Apply(F f) {
    for (double& x : span()) x = f(x);
    return *this;
  }

  friend bool operator==(const MjArray& a, const MjArray& b);

 private:
  struct Uninitialized {};
  MjArray(std::size_t size, Uninitialized);

  template <class Op>
  MjArray& ElementWise(const MjArray& rhs, Op op) {
    if (rhs.size_ != size_) ThrowSizeMismatch();
    for (std::size_t i = 0; i < size_; ++i) data_[i] = op(data_[i], rhs[i]);
    return *this;
  }
  [[noreturn]] static void ThrowSizeMismatch();

  std::unique_ptr<double[]> data_;
  std::size_t size_ = 0;
  std::size_t capacity_ = 0;
};

MjArray operator+(MjArray lhs, const MjArray& rhs);
MjArray operator-(MjArray lhs, const MjArray& rhs);
MjArray operator*(MjArray lhs, const MjArray& rhs);
MjArray operator*(MjArray lhs, double rhs);
MjArray operator*(double lhs, MjArray rhs);

}  // namespace cpp4q::joshi

#endif  // CPP4Q_JOSHI_APPENDIX_C_ARRAY_MJ_ARRAY_H_
