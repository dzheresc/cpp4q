// Chapter 17: An Overview of Templates.
//
// Function templates (constrained with C++20 concepts), class templates
// with non-type parameters, and template argument deduction. Three
// examples a quant uses: a numerical integrator that inlines whatever
// callable it is given, a fixed-size matrix whose shapes are checked at
// compile time (and which works in constant expressions), and a linear
// interpolator that serves as a discount or volatility curve for any
// floating-point type.

#ifndef CPP4Q_ARMSTRONG_CH17_TEMPLATES_TEMPLATES_H_
#define CPP4Q_ARMSTRONG_CH17_TEMPLATES_TEMPLATES_H_

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace cpp4q::armstrong::ch17 {

template <typename F, typename T>
concept RealFunctionOf = std::floating_point<T> && std::invocable<F, T> &&
                         std::convertible_to<std::invoke_result_t<F, T>, T>;

// Composite Simpson's rule with `intervals` (rounded up to even) panels.
template <std::floating_point T, RealFunctionOf<T> F>
T IntegrateSimpson(F&& f, T a, T b, int intervals) {
  if (intervals < 2) throw std::invalid_argument("need >= 2 intervals");
  if (intervals % 2 != 0) ++intervals;
  const T h = (b - a) / static_cast<T>(intervals);
  T odd = 0, even = 0;
  for (int i = 1; i < intervals; ++i) {
    const T fx = f(a + static_cast<T>(i) * h);
    (i % 2 != 0 ? odd : even) += fx;
  }
  return h / 3 * (f(a) + f(b) + 4 * odd + 2 * even);
}

// Sum of any range of arithmetic values, accumulated in T.
template <typename T = double, std::ranges::input_range R>
  requires std::is_arithmetic_v<std::ranges::range_value_t<R>>
T SumOf(R&& range) {
  T total{};
  for (const auto& x : range) total += static_cast<T>(x);
  return total;
}

// A Rows x Cols matrix stored inline (row-major). Mismatched products
// fail to compile instead of throwing.
template <typename T, std::size_t Rows, std::size_t Cols>
class FixedMatrix {
 public:
  constexpr FixedMatrix() = default;
  constexpr explicit FixedMatrix(const std::array<T, Rows * Cols>& values)
      : data_(values) {}

  static constexpr FixedMatrix Identity()
    requires(Rows == Cols)
  {
    FixedMatrix m;
    for (std::size_t i = 0; i < Rows; ++i) m(i, i) = T{1};
    return m;
  }

  static constexpr std::size_t rows() { return Rows; }
  static constexpr std::size_t cols() { return Cols; }
  constexpr T& operator()(std::size_t i, std::size_t j) {
    return data_[i * Cols + j];
  }
  constexpr const T& operator()(std::size_t i, std::size_t j) const {
    return data_[i * Cols + j];
  }

  template <std::size_t Inner>
  constexpr FixedMatrix<T, Rows, Inner> operator*(
      const FixedMatrix<T, Cols, Inner>& rhs) const {
    FixedMatrix<T, Rows, Inner> out;
    for (std::size_t i = 0; i < Rows; ++i) {
      for (std::size_t k = 0; k < Cols; ++k) {
        for (std::size_t j = 0; j < Inner; ++j) {
          out(i, j) += (*this)(i, k) * rhs(k, j);
        }
      }
    }
    return out;
  }

  constexpr FixedMatrix<T, Cols, Rows> Transpose() const {
    FixedMatrix<T, Cols, Rows> out;
    for (std::size_t i = 0; i < Rows; ++i) {
      for (std::size_t j = 0; j < Cols; ++j) out(j, i) = (*this)(i, j);
    }
    return out;
  }

  constexpr bool operator==(const FixedMatrix&) const = default;

 private:
  std::array<T, Rows * Cols> data_{};
};

// Piecewise-linear y(x) through strictly increasing knots, flat outside.
template <std::floating_point T>
class LinearInterpolator {
 public:
  LinearInterpolator(std::vector<T> x, std::vector<T> y)
      : x_(std::move(x)), y_(std::move(y)) {
    if (x_.empty() || x_.size() != y_.size()) {
      throw std::invalid_argument("need equal, non-zero numbers of knots");
    }
    for (std::size_t i = 1; i < x_.size(); ++i) {
      if (!(x_[i] > x_[i - 1])) {
        throw std::invalid_argument("knots must be strictly increasing");
      }
    }
  }

  T operator()(T x) const {
    if (x <= x_.front()) return y_.front();
    if (x >= x_.back()) return y_.back();
    const auto hi = static_cast<std::size_t>(
        std::ranges::upper_bound(x_, x) - x_.begin());
    const std::size_t lo = hi - 1;
    const T w = (x - x_[lo]) / (x_[hi] - x_[lo]);
    return y_[lo] + w * (y_[hi] - y_[lo]);
  }

 private:
  std::vector<T> x_;
  std::vector<T> y_;
};

// Class template argument deduction: LinearInterpolator(xs, ys) deduces T.
template <typename T>
LinearInterpolator(std::vector<T>, std::vector<T>) -> LinearInterpolator<T>;

}  // namespace cpp4q::armstrong::ch17

#endif  // CPP4Q_ARMSTRONG_CH17_TEMPLATES_TEMPLATES_H_
