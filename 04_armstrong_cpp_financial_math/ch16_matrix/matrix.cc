#include "ch16_matrix/matrix.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace cpp4q::armstrong::ch16 {

Matrix::Matrix(std::size_t rows, std::size_t cols, double fill)
    : rows_(rows),
      cols_(cols),
      data_(std::make_unique_for_overwrite<double[]>(rows * cols)) {
  std::fill_n(data_.get(), size(), fill);
}

Matrix::Matrix(std::initializer_list<std::initializer_list<double>> rows)
    : Matrix(rows.size(), rows.size() == 0 ? 0 : rows.begin()->size()) {
  std::size_t i = 0;
  for (const auto& row : rows) {
    if (row.size() != cols_) throw std::invalid_argument("ragged rows");
    std::size_t j = 0;
    for (double x : row) (*this)(i, j++) = x;
    ++i;
  }
}

Matrix Matrix::Identity(std::size_t n) {
  Matrix m(n, n);
  for (std::size_t i = 0; i < n; ++i) m(i, i) = 1.0;
  return m;
}

Matrix::Matrix(const Matrix& other)
    : rows_(other.rows_),
      cols_(other.cols_),
      data_(std::make_unique_for_overwrite<double[]>(other.size())) {
  std::copy_n(other.data_.get(), size(), data_.get());
}

Matrix& Matrix::operator=(const Matrix& other) {
  if (this != &other) {
    Matrix copy(other);  // may throw; *this is untouched if it does
    swap(copy);
  }
  return *this;
}

Matrix::Matrix(Matrix&& other) noexcept
    : rows_(std::exchange(other.rows_, 0)),
      cols_(std::exchange(other.cols_, 0)),
      data_(std::move(other.data_)) {}

Matrix& Matrix::operator=(Matrix&& other) noexcept {
  rows_ = std::exchange(other.rows_, 0);
  cols_ = std::exchange(other.cols_, 0);
  data_ = std::move(other.data_);
  return *this;
}

void Matrix::swap(Matrix& other) noexcept {
  std::swap(rows_, other.rows_);
  std::swap(cols_, other.cols_);
  std::swap(data_, other.data_);
}

double& Matrix::at(std::size_t i, std::size_t j) {
  if (i >= rows_ || j >= cols_) throw std::out_of_range("Matrix::at");
  return (*this)(i, j);
}

double Matrix::at(std::size_t i, std::size_t j) const {
  if (i >= rows_ || j >= cols_) throw std::out_of_range("Matrix::at");
  return (*this)(i, j);
}

void Matrix::RequireSameShape(const Matrix& rhs) const {
  if (rows_ != rhs.rows_ || cols_ != rhs.cols_) {
    throw std::invalid_argument("matrix shapes differ");
  }
}

Matrix& Matrix::operator+=(const Matrix& rhs) {
  RequireSameShape(rhs);
  for (std::size_t k = 0; k < size(); ++k) data_[k] += rhs.data_[k];
  return *this;
}

Matrix& Matrix::operator-=(const Matrix& rhs) {
  RequireSameShape(rhs);
  for (std::size_t k = 0; k < size(); ++k) data_[k] -= rhs.data_[k];
  return *this;
}

Matrix& Matrix::operator*=(double scalar) {
  for (std::size_t k = 0; k < size(); ++k) data_[k] *= scalar;
  return *this;
}

Matrix Matrix::Transpose() const {
  Matrix t(cols_, rows_);
  for (std::size_t j = 0; j < cols_; ++j) {
    for (std::size_t i = 0; i < rows_; ++i) t(j, i) = (*this)(i, j);
  }
  return t;
}

Matrix operator+(Matrix lhs, const Matrix& rhs) { return lhs += rhs; }
Matrix operator-(Matrix lhs, const Matrix& rhs) { return lhs -= rhs; }
Matrix operator*(Matrix lhs, double scalar) { return lhs *= scalar; }
Matrix operator*(double scalar, Matrix rhs) { return rhs *= scalar; }

Matrix operator*(const Matrix& a, const Matrix& b) {
  if (a.cols() != b.rows()) {
    throw std::invalid_argument("inner dimensions differ");
  }
  // j-k-i order: the inner loop runs down one column of a and of c, both
  // contiguous in column-major storage (and so vectorisable). The
  // textbook i-j-k order walks a row of a with a stride of a.rows().
  Matrix c(a.rows(), b.cols());
  const std::size_t m = a.rows();
  for (std::size_t j = 0; j < b.cols(); ++j) {
    double* c_col = c.data() + j * m;
    for (std::size_t k = 0; k < a.cols(); ++k) {
      const double b_kj = b(k, j);
      const double* a_col = a.data() + k * m;
      for (std::size_t i = 0; i < m; ++i) c_col[i] += a_col[i] * b_kj;
    }
  }
  return c;
}

std::vector<double> operator*(const Matrix& a, std::span<const double> x) {
  if (a.cols() != x.size()) throw std::invalid_argument("size mismatch");
  std::vector<double> y(a.rows(), 0.0);
  for (std::size_t j = 0; j < a.cols(); ++j) {
    const double xj = x[j];
    for (std::size_t i = 0; i < a.rows(); ++i) y[i] += a(i, j) * xj;
  }
  return y;
}

bool operator==(const Matrix& a, const Matrix& b) {
  return a.rows() == b.rows() && a.cols() == b.cols() &&
         std::equal(a.data(), a.data() + a.size(), b.data());
}

double MaxAbsDifference(const Matrix& a, const Matrix& b) {
  if (a.rows() != b.rows() || a.cols() != b.cols()) {
    throw std::invalid_argument("matrix shapes differ");
  }
  double m = 0.0;
  for (std::size_t k = 0; k < a.size(); ++k) {
    m = std::max(m, std::abs(a.data()[k] - b.data()[k]));
  }
  return m;
}

Matrix Cholesky(const Matrix& a) {
  const std::size_t n = a.rows();
  if (a.cols() != n) throw std::invalid_argument("Cholesky needs square");
  double scale = 0.0;
  for (std::size_t k = 0; k < a.size(); ++k) {
    scale = std::max(scale, std::abs(a.data()[k]));
  }
  const double sym_tol = 1e-12 * std::max(scale, 1.0);
  for (std::size_t j = 0; j < n; ++j) {
    for (std::size_t i = j + 1; i < n; ++i) {
      if (std::abs(a(i, j) - a(j, i)) > sym_tol) {
        throw std::domain_error("Cholesky needs a symmetric matrix");
      }
    }
  }
  // Left-looking, column by column: column j of L is column j of a minus
  // L(j, k) times column k of L for each earlier k. Every inner loop runs
  // down a contiguous column (the row-oriented form strides by n).
  Matrix l(n, n);
  for (std::size_t j = 0; j < n; ++j) {
    double* col_j = l.data() + j * n;
    for (std::size_t i = j; i < n; ++i) col_j[i] = a(i, j);
    for (std::size_t k = 0; k < j; ++k) {
      const double* col_k = l.data() + k * n;
      const double ljk = col_k[j];
      for (std::size_t i = j; i < n; ++i) col_j[i] -= col_k[i] * ljk;
    }
    const double d = col_j[j];
    if (!(d > 0)) throw std::domain_error("matrix is not positive definite");
    const double ljj = std::sqrt(d);
    col_j[j] = ljj;
    for (std::size_t i = j + 1; i < n; ++i) col_j[i] /= ljj;
  }
  return l;
}

std::vector<double> Solve(const Matrix& a, std::span<const double> b) {
  const std::size_t n = a.rows();
  if (a.cols() != n || b.size() != n) {
    throw std::invalid_argument("Solve needs square a and matching b");
  }
  Matrix lu(a);
  std::vector<double> x(b.begin(), b.end());
  double scale = 0.0;
  for (std::size_t k = 0; k < a.size(); ++k) {
    scale = std::max(scale, std::abs(a.data()[k]));
  }
  const double tiny = scale * static_cast<double>(n) *
                      std::numeric_limits<double>::epsilon();
  for (std::size_t k = 0; k < n; ++k) {
    std::size_t pivot = k;
    for (std::size_t i = k + 1; i < n; ++i) {
      if (std::abs(lu(i, k)) > std::abs(lu(pivot, k))) pivot = i;
    }
    if (!(std::abs(lu(pivot, k)) > tiny)) {
      throw std::domain_error("matrix is singular");
    }
    if (pivot != k) {
      for (std::size_t j = 0; j < n; ++j) std::swap(lu(k, j), lu(pivot, j));
      std::swap(x[k], x[pivot]);
    }
    // Column-oriented elimination (right-looking): scale column k below
    // the pivot, then subtract multiples of it from each later column.
    // Every inner loop is contiguous in column-major storage.
    double* col_k = lu.data() + k * n;
    const double inv_pivot = 1.0 / col_k[k];
    for (std::size_t i = k + 1; i < n; ++i) col_k[i] *= inv_pivot;
    for (std::size_t j = k + 1; j < n; ++j) {
      double* col_j = lu.data() + j * n;
      const double f = col_j[k];
      if (f == 0.0) continue;
      for (std::size_t i = k + 1; i < n; ++i) col_j[i] -= col_k[i] * f;
    }
    const double xk = x[k];
    for (std::size_t i = k + 1; i < n; ++i) x[i] -= col_k[i] * xk;
  }
  // Back substitution, also by columns.
  for (std::size_t k = n; k-- > 0;) {
    const double* col_k = lu.data() + k * n;
    x[k] /= col_k[k];
    const double xk = x[k];
    for (std::size_t i = 0; i < k; ++i) x[i] -= col_k[i] * xk;
  }
  return x;
}

std::ostream& operator<<(std::ostream& out, const Matrix& m) {
  for (std::size_t i = 0; i < m.rows(); ++i) {
    out << (i == 0 ? "[" : " ");
    for (std::size_t j = 0; j < m.cols(); ++j) {
      out << (j == 0 ? "" : ", ") << m(i, j);
    }
    out << (i + 1 == m.rows() ? "]\n" : ";\n");
  }
  return out;
}

}  // namespace cpp4q::armstrong::ch16
