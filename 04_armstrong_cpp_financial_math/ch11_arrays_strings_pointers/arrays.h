// Chapter 11: Arrays, Strings, and Pointers.
//
// The low-level layer under std::vector and std::string: raw arrays and
// pointer arithmetic, C strings, new[] / delete[] (and the RAII type that
// makes them safe), and std::span / std::string_view as the modern way to
// pass "pointer plus length". The financial thread is reading a CSV of
// closing prices and turning it into log returns and historical
// volatility.

#ifndef CPP4Q_ARMSTRONG_CH11_ARRAYS_STRINGS_POINTERS_ARRAYS_H_
#define CPP4Q_ARMSTRONG_CH11_ARRAYS_STRINGS_POINTERS_ARRAYS_H_

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cpp4q::armstrong::ch11 {

// Classic pointer-and-length interfaces.
double SumArray(const double* values, std::size_t n);
// Reverses [first, last) in place by walking two pointers inwards.
void ReverseInPlace(double* first, double* last);
// strlen, written out.
std::size_t CStringLength(const char* s);
// Copies at most capacity - 1 characters and always NUL-terminates
// (unlike strncpy). Returns the length of src, so truncation shows as a
// return value >= capacity (strlcpy semantics).
std::size_t CopyCString(char* dest, std::size_t capacity, const char* src);

// A fixed-size heap array owning its storage through unique_ptr<T[]>:
// what new[] / delete[] look like once the delete cannot be forgotten.
// Movable, not copyable (copying is chapter 12).
class DoubleBuffer {
 public:
  explicit DoubleBuffer(std::size_t n)
      : data_(std::make_unique<double[]>(n)), size_(n) {}
  std::size_t size() const { return size_; }
  double* data() { return data_.get(); }
  const double* data() const { return data_.get(); }
  double& operator[](std::size_t i) { return data_[i]; }
  double operator[](std::size_t i) const { return data_[i]; }
  std::span<double> span() { return {data_.get(), size_}; }
  std::span<const double> span() const { return {data_.get(), size_}; }

 private:
  std::unique_ptr<double[]> data_;
  std::size_t size_;
};

// Splits on `delimiter`; empty fields are kept. The views point into
// `text`, which must outlive them.
std::vector<std::string_view> Split(std::string_view text, char delimiter);
// Removes leading and trailing spaces, tabs and \r.
std::string_view Trim(std::string_view text);

struct PriceRecord {
  std::string date;
  double close = 0.0;
};

// Reads "date,close" lines (one header line, which is skipped). Blank
// lines are ignored; a malformed line throws std::runtime_error naming
// its line number.
std::vector<PriceRecord> ReadPriceCsv(std::istream& in);

// log(p[i+1] / p[i]); needs size >= 2 and positive prices.
std::vector<double> LogReturns(std::span<const double> prices);
// Sample std of the log returns times sqrt(periods_per_year).
double HistoricalVolatility(std::span<const double> prices,
                            double periods_per_year = 252.0);

}  // namespace cpp4q::armstrong::ch11

#endif  // CPP4Q_ARMSTRONG_CH11_ARRAYS_STRINGS_POINTERS_ARRAYS_H_
