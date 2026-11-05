#include "ch02_classes/price_buffer.h"

#include <algorithm>
#include <utility>

namespace cpp4q::hanson::ch02 {

PriceBuffer::PriceBuffer(std::size_t size)
    : data_(std::make_unique<double[]>(size)), size_(size) {}

PriceBuffer::PriceBuffer(std::initializer_list<double> values)
    : data_(std::make_unique_for_overwrite<double[]>(values.size())),
      size_(values.size()) {
  std::ranges::copy(values, data_.get());
}

PriceBuffer::PriceBuffer(const PriceBuffer& other)
    : data_(other.size_ ? std::make_unique_for_overwrite<double[]>(other.size_)
                        : nullptr),
      size_(other.size_) {
  std::ranges::copy(other.view(), data_.get());
}

PriceBuffer& PriceBuffer::operator=(const PriceBuffer& other) {
  if (this != &other) {
    if (size_ == other.size_) {
      // Reuse the existing allocation.
      std::ranges::copy(other.view(), data_.get());
    } else {
      PriceBuffer tmp(other);  // strong exception guarantee
      swap(*this, tmp);
    }
  }
  return *this;
}

PriceBuffer::PriceBuffer(PriceBuffer&& other) noexcept
    : data_(std::move(other.data_)), size_(std::exchange(other.size_, 0)) {}

PriceBuffer& PriceBuffer::operator=(PriceBuffer&& other) noexcept {
  data_ = std::move(other.data_);
  size_ = std::exchange(other.size_, 0);
  return *this;
}

void swap(PriceBuffer& a, PriceBuffer& b) noexcept {
  using std::swap;
  swap(a.data_, b.data_);
  swap(a.size_, b.size_);
}

bool operator==(const PriceBuffer& a, const PriceBuffer& b) {
  return std::ranges::equal(a.view(), b.view());
}

}  // namespace cpp4q::hanson::ch02
