#include "appendix_c_array/mj_array.h"

#include <algorithm>
#include <functional>
#include <stdexcept>

namespace cpp4q::joshi {
MjArray::MjArray(std::size_t size, Uninitialized)
    : data_(size ? std::make_unique_for_overwrite<double[]>(size) : nullptr),
      size_(size),
      capacity_(size) {}

MjArray::MjArray(std::size_t size, double value)
    : MjArray(size, Uninitialized{}) {
  std::fill_n(data_.get(), size_, value);
}

MjArray::MjArray(std::initializer_list<double> values)
    : MjArray(values.size(), Uninitialized{}) {
  std::copy(values.begin(), values.end(), data_.get());
}

// Copies straight into fresh storage. (The first version delegated to
// MjArray(size), which zero-filled the buffer before the copy overwrote
// it: a fifth of the `array` workload. See profiling/REPORT.md.)
MjArray::MjArray(const MjArray& other) : MjArray(other.size_, Uninitialized{}) {
  std::copy_n(other.data_.get(), size_, data_.get());
}

MjArray& MjArray::operator=(const MjArray& other) {
  if (this == &other) return *this;
  resize(other.size_);  // reuses the buffer when it is big enough
  std::copy_n(other.data_.get(), size_, data_.get());
  return *this;
}

MjArray& MjArray::operator=(double value) {
  std::fill_n(data_.get(), size_, value);
  return *this;
}

void MjArray::resize(std::size_t new_size) {
  if (new_size > capacity_) {
    data_ = std::make_unique_for_overwrite<double[]>(new_size);
    capacity_ = new_size;
  }
  size_ = new_size;
}

void MjArray::ThrowSizeMismatch() {
  throw std::invalid_argument("MjArray: size mismatch");
}

MjArray& MjArray::operator+=(const MjArray& rhs) {
  return ElementWise(rhs, std::plus<>{});
}
MjArray& MjArray::operator-=(const MjArray& rhs) {
  return ElementWise(rhs, std::minus<>{});
}
MjArray& MjArray::operator*=(const MjArray& rhs) {
  return ElementWise(rhs, std::multiplies<>{});
}
MjArray& MjArray::operator/=(const MjArray& rhs) {
  return ElementWise(rhs, std::divides<>{});
}
MjArray& MjArray::operator+=(double rhs) {
  return Apply([rhs](double x) { return x + rhs; });
}
MjArray& MjArray::operator-=(double rhs) {
  return Apply([rhs](double x) { return x - rhs; });
}
MjArray& MjArray::operator*=(double rhs) {
  return Apply([rhs](double x) { return x * rhs; });
}
MjArray& MjArray::operator/=(double rhs) {
  return Apply([rhs](double x) { return x / rhs; });
}

MjArray& MjArray::AddScaled(double a, const MjArray& x) {
  if (x.size_ != size_) ThrowSizeMismatch();
  double* y = data_.get();
  const double* xs = x.data_.get();
  for (std::size_t i = 0; i < size_; ++i) y[i] += a * xs[i];
  return *this;
}

double MjArray::Sum() const {
  double s = 0.0;
  for (const double x : span()) s += x;
  return s;
}

double MjArray::Min() const {
  if (empty()) throw std::logic_error("MjArray::Min of an empty array");
  return *std::min_element(begin(), end());
}

double MjArray::Max() const {
  if (empty()) throw std::logic_error("MjArray::Max of an empty array");
  return *std::max_element(begin(), end());
}

bool operator==(const MjArray& a, const MjArray& b) {
  return std::ranges::equal(a.span(), b.span());
}

MjArray operator+(MjArray lhs, const MjArray& rhs) { return lhs += rhs; }
MjArray operator-(MjArray lhs, const MjArray& rhs) { return lhs -= rhs; }
MjArray operator*(MjArray lhs, const MjArray& rhs) { return lhs *= rhs; }
MjArray operator*(MjArray lhs, double rhs) { return lhs *= rhs; }
MjArray operator*(double lhs, MjArray rhs) { return rhs *= lhs; }

}  // namespace cpp4q::joshi
