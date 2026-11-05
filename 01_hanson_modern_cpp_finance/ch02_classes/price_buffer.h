// Chapter 2 / Appendix C: the special member functions.
//
// PriceBuffer owns a heap array, so it spells out all five special members.
// In production you would just hold a std::vector<double> (rule of zero);
// this class exists to show what copy and move actually do.

#ifndef CPP4Q_HANSON_CH02_CLASSES_PRICE_BUFFER_H_
#define CPP4Q_HANSON_CH02_CLASSES_PRICE_BUFFER_H_

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <span>

namespace cpp4q::hanson::ch02 {

class PriceBuffer {
 public:
  PriceBuffer() = default;
  explicit PriceBuffer(std::size_t size);  // zero-initialised
  PriceBuffer(std::initializer_list<double> values);

  // Deep copy.
  PriceBuffer(const PriceBuffer& other);
  PriceBuffer& operator=(const PriceBuffer& other);

  // Steal the pointer; leave `other` empty but valid. noexcept so that
  // std::vector<PriceBuffer> moves rather than copies on reallocation.
  PriceBuffer(PriceBuffer&& other) noexcept;
  PriceBuffer& operator=(PriceBuffer&& other) noexcept;

  ~PriceBuffer() = default;  // unique_ptr<double[]> releases the array

  [[nodiscard]] std::size_t size() const { return size_; }
  [[nodiscard]] bool empty() const { return size_ == 0; }
  double& operator[](std::size_t i) { return data_[i]; }
  double operator[](std::size_t i) const { return data_[i]; }
  [[nodiscard]] std::span<const double> view() const {
    return {data_.get(), size_};
  }

  friend void swap(PriceBuffer& a, PriceBuffer& b) noexcept;
  friend bool operator==(const PriceBuffer& a, const PriceBuffer& b);

 private:
  std::unique_ptr<double[]> data_;
  std::size_t size_ = 0;
};

}  // namespace cpp4q::hanson::ch02

#endif  // CPP4Q_HANSON_CH02_CLASSES_PRICE_BUFFER_H_
