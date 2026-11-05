// Chapter 8: Linear Algebra.
//
// A small dense row-major matrix. The book uses Eigen; writing the kernels
// by hand makes the memory-access trade-offs visible to the profiler
// (see bench/ and profiling/).

#ifndef CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_MATRIX_H_
#define CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_MATRIX_H_

#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <span>
#include <vector>

namespace cpp4q::hanson::ch08 {

class Matrix {
 public:
  Matrix() = default;
  Matrix(std::size_t rows, std::size_t cols, double fill = 0.0);
  // Row-wise literal: Matrix{{1, 2}, {3, 4}}. Throws on ragged rows.
  Matrix(std::initializer_list<std::initializer_list<double>> rows);

  static Matrix Identity(std::size_t n);

  [[nodiscard]] std::size_t rows() const { return rows_; }
  [[nodiscard]] std::size_t cols() const { return cols_; }

  double& operator()(std::size_t i, std::size_t j) {
    return data_[i * cols_ + j];
  }
  double operator()(std::size_t i, std::size_t j) const {
    return data_[i * cols_ + j];
  }
  [[nodiscard]] std::span<double> row(std::size_t i) {
    return {data_.data() + i * cols_, cols_};
  }
  [[nodiscard]] std::span<const double> row(std::size_t i) const {
    return {data_.data() + i * cols_, cols_};
  }
  [[nodiscard]] std::span<const double> data() const { return data_; }

  [[nodiscard]] Matrix Transpose() const;

  Matrix& operator+=(const Matrix& rhs);
  Matrix& operator-=(const Matrix& rhs);
  Matrix& operator*=(double s);

  friend Matrix operator+(Matrix a, const Matrix& b) { return a += b; }
  friend Matrix operator-(Matrix a, const Matrix& b) { return a -= b; }
  friend Matrix operator*(Matrix a, double s) { return a *= s; }
  friend Matrix operator*(double s, Matrix a) { return a *= s; }
  friend bool operator==(const Matrix&, const Matrix&) = default;
  friend std::ostream& operator<<(std::ostream& os, const Matrix& m);

 private:
  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::vector<double> data_;
};

// Textbook i-j-k triple loop: the inner loop strides down a column of `b`.
Matrix MultiplyNaive(const Matrix& a, const Matrix& b);
// Cache-friendly (i-k-j, k-tiled) product used by operator*.
Matrix Multiply(const Matrix& a, const Matrix& b);
inline Matrix operator*(const Matrix& a, const Matrix& b) {
  return Multiply(a, b);
}

std::vector<double> Multiply(const Matrix& a, std::span<const double> x);

double MaxAbsDiff(const Matrix& a, const Matrix& b);

}  // namespace cpp4q::hanson::ch08

#endif  // CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_MATRIX_H_
