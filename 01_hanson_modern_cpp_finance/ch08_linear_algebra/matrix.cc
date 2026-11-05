#include "ch08_linear_algebra/matrix.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <functional>
#include <numeric>
#include <stdexcept>

namespace cpp4q::hanson::ch08 {
namespace {

void RequireSameShape(const Matrix& a, const Matrix& b) {
  if (a.rows() != b.rows() || a.cols() != b.cols()) {
    throw std::invalid_argument("Matrix: shape mismatch");
  }
}

void RequireConformable(const Matrix& a, const Matrix& b) {
  if (a.cols() != b.rows()) {
    throw std::invalid_argument("Matrix: inner dimensions differ");
  }
}

}  // namespace

Matrix::Matrix(std::size_t rows, std::size_t cols, double fill)
    : rows_(rows), cols_(cols), data_(rows * cols, fill) {}

Matrix::Matrix(std::initializer_list<std::initializer_list<double>> rows)
    : rows_(rows.size()), cols_(rows.size() ? rows.begin()->size() : 0) {
  data_.reserve(rows_ * cols_);
  for (const auto& r : rows) {
    if (r.size() != cols_) throw std::invalid_argument("Matrix: ragged rows");
    data_.insert(data_.end(), r.begin(), r.end());
  }
}

Matrix Matrix::Identity(std::size_t n) {
  Matrix m(n, n);
  for (std::size_t i = 0; i < n; ++i) m(i, i) = 1.0;
  return m;
}

Matrix Matrix::Transpose() const {
  Matrix t(cols_, rows_);
  for (std::size_t i = 0; i < rows_; ++i) {
    for (std::size_t j = 0; j < cols_; ++j) t(j, i) = (*this)(i, j);
  }
  return t;
}

Matrix& Matrix::operator+=(const Matrix& rhs) {
  RequireSameShape(*this, rhs);
  std::ranges::transform(data_, rhs.data_, data_.begin(), std::plus<>());
  return *this;
}

Matrix& Matrix::operator-=(const Matrix& rhs) {
  RequireSameShape(*this, rhs);
  std::ranges::transform(data_, rhs.data_, data_.begin(), std::minus<>());
  return *this;
}

Matrix& Matrix::operator*=(double s) {
  for (double& x : data_) x *= s;
  return *this;
}

std::ostream& operator<<(std::ostream& os, const Matrix& m) {
  for (std::size_t i = 0; i < m.rows(); ++i) {
    for (std::size_t j = 0; j < m.cols(); ++j) {
      os << std::format("{:10.4f}", m(i, j));
    }
    os << '\n';
  }
  return os;
}

Matrix MultiplyNaive(const Matrix& a, const Matrix& b) {
  RequireConformable(a, b);
  Matrix c(a.rows(), b.cols());
  for (std::size_t i = 0; i < a.rows(); ++i) {
    for (std::size_t j = 0; j < b.cols(); ++j) {
      double sum = 0.0;
      for (std::size_t k = 0; k < a.cols(); ++k) sum += a(i, k) * b(k, j);
      c(i, j) = sum;
    }
  }
  return c;
}

Matrix Multiply(const Matrix& a, const Matrix& b) {
  // Loop order i-k-j: the inner loop walks a row of `b` and a row of `c`
  // contiguously (unit stride, auto-vectorisable) instead of striding down a
  // column of `b`. The k loop is tiled so a kBlock x n panel of `b` stays in
  // cache while every row of `a` is streamed past it.
  RequireConformable(a, b);
  constexpr std::size_t kBlock = 64;
  const std::size_t n = a.rows();
  const std::size_t inner = a.cols();
  const std::size_t m = b.cols();
  Matrix c(n, m);
  for (std::size_t kk = 0; kk < inner; kk += kBlock) {
    const std::size_t k_end = std::min(kk + kBlock, inner);
    for (std::size_t i = 0; i < n; ++i) {
      double* __restrict c_row = c.row(i).data();
      for (std::size_t k = kk; k < k_end; ++k) {
        const double a_ik = a(i, k);
        const double* __restrict b_row = b.row(k).data();
        for (std::size_t j = 0; j < m; ++j) c_row[j] += a_ik * b_row[j];
      }
    }
  }
  return c;
}

std::vector<double> Multiply(const Matrix& a, std::span<const double> x) {
  if (a.cols() != x.size()) {
    throw std::invalid_argument("Matrix*vector: size mismatch");
  }
  std::vector<double> y(a.rows());
  for (std::size_t i = 0; i < a.rows(); ++i) {
    const auto r = a.row(i);
    y[i] = std::inner_product(r.begin(), r.end(), x.begin(), 0.0);
  }
  return y;
}

double MaxAbsDiff(const Matrix& a, const Matrix& b) {
  RequireSameShape(a, b);
  double worst = 0.0;
  for (std::size_t i = 0; i < a.data().size(); ++i) {
    worst = std::max(worst, std::abs(a.data()[i] - b.data()[i]));
  }
  return worst;
}

}  // namespace cpp4q::hanson::ch08
