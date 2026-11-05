// Chapter 16: A Matrix Class.
//
// A dense column-major matrix of doubles that owns a raw heap buffer, so
// that it has to implement the rule of five itself: copy constructor and
// copy assignment (deep copy, strong exception guarantee by
// copy-and-swap), move constructor and move assignment (steal the buffer,
// noexcept), and the destructor (via unique_ptr<double[]>). On top of
// that: element access, arithmetic operators, transpose, Cholesky
// factorisation and a partially pivoted LU solve.

#ifndef CPP4Q_ARMSTRONG_CH16_MATRIX_MATRIX_H_
#define CPP4Q_ARMSTRONG_CH16_MATRIX_MATRIX_H_

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <span>
#include <vector>

namespace cpp4q::armstrong::ch16 {

class Matrix {
 public:
  Matrix() = default;
  Matrix(std::size_t rows, std::size_t cols, double fill = 0.0);
  // Row-wise literal: {{1, 2}, {3, 4}}. Throws if rows are ragged.
  Matrix(std::initializer_list<std::initializer_list<double>> rows);
  static Matrix Identity(std::size_t n);

  Matrix(const Matrix& other);
  Matrix& operator=(const Matrix& other);
  Matrix(Matrix&& other) noexcept;
  Matrix& operator=(Matrix&& other) noexcept;
  ~Matrix() = default;

  std::size_t rows() const { return rows_; }
  std::size_t cols() const { return cols_; }
  std::size_t size() const { return rows_ * cols_; }

  // Unchecked element access (column-major: element (i, j) is at
  // data()[j * rows() + i]).
  double& operator()(std::size_t i, std::size_t j) {
    return data_[j * rows_ + i];
  }
  double operator()(std::size_t i, std::size_t j) const {
    return data_[j * rows_ + i];
  }
  // Bounds-checked: throws std::out_of_range.
  double& at(std::size_t i, std::size_t j);
  double at(std::size_t i, std::size_t j) const;

  double* data() { return data_.get(); }
  const double* data() const { return data_.get(); }
  // Column j as a contiguous span.
  std::span<const double> column(std::size_t j) const {
    return {data_.get() + j * rows_, rows_};
  }

  // Throw std::invalid_argument on a shape mismatch.
  Matrix& operator+=(const Matrix& rhs);
  Matrix& operator-=(const Matrix& rhs);
  Matrix& operator*=(double scalar);

  Matrix Transpose() const;
  void swap(Matrix& other) noexcept;

 private:
  void RequireSameShape(const Matrix& rhs) const;
  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::unique_ptr<double[]> data_;
};

inline void swap(Matrix& a, Matrix& b) noexcept { a.swap(b); }

Matrix operator+(Matrix lhs, const Matrix& rhs);
Matrix operator-(Matrix lhs, const Matrix& rhs);
Matrix operator*(Matrix lhs, double scalar);
Matrix operator*(double scalar, Matrix rhs);
// Matrix product; throws on inner-dimension mismatch.
Matrix operator*(const Matrix& a, const Matrix& b);
std::vector<double> operator*(const Matrix& a, std::span<const double> x);
bool operator==(const Matrix& a, const Matrix& b);

// max |a(i,j) - b(i,j)|; throws on shape mismatch.
double MaxAbsDifference(const Matrix& a, const Matrix& b);

// Lower-triangular L with L L^T = a. Throws std::domain_error if a is not
// symmetric positive definite (to within a relative tolerance).
Matrix Cholesky(const Matrix& a);

// Solves a x = b by LU decomposition with partial pivoting. Throws
// std::domain_error if a is (numerically) singular.
std::vector<double> Solve(const Matrix& a, std::span<const double> b);

std::ostream& operator<<(std::ostream& out, const Matrix& m);

}  // namespace cpp4q::armstrong::ch16

#endif  // CPP4Q_ARMSTRONG_CH16_MATRIX_MATRIX_H_
