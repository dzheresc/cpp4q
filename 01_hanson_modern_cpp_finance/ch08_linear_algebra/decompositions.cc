#include "ch08_linear_algebra/decompositions.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace cpp4q::hanson::ch08 {

Matrix Cholesky(const Matrix& a) {
  const std::size_t n = a.rows();
  if (a.cols() != n) throw std::domain_error("Cholesky: matrix not square");
  Matrix l(n, n);
  for (std::size_t j = 0; j < n; ++j) {
    for (std::size_t i = 0; i < j; ++i) {
      if (std::abs(a(i, j) - a(j, i)) > 1e-12 * (1.0 + std::abs(a(i, j)))) {
        throw std::domain_error("Cholesky: matrix not symmetric");
      }
    }
    double diag = a(j, j);
    for (std::size_t k = 0; k < j; ++k) diag -= l(j, k) * l(j, k);
    if (!(diag > 0.0)) {
      throw std::domain_error("Cholesky: matrix not positive definite");
    }
    l(j, j) = std::sqrt(diag);
    for (std::size_t i = j + 1; i < n; ++i) {
      double s = a(i, j);
      for (std::size_t k = 0; k < j; ++k) s -= l(i, k) * l(j, k);
      l(i, j) = s / l(j, j);
    }
  }
  return l;
}

LuDecomposition::LuDecomposition(Matrix a) : lu_(std::move(a)) {
  const std::size_t n = lu_.rows();
  if (lu_.cols() != n) throw std::domain_error("LU: matrix not square");
  perm_.resize(n);
  std::iota(perm_.begin(), perm_.end(), std::size_t{0});
  for (std::size_t k = 0; k < n; ++k) {
    std::size_t pivot = k;
    for (std::size_t i = k + 1; i < n; ++i) {
      if (std::abs(lu_(i, k)) > std::abs(lu_(pivot, k))) pivot = i;
    }
    if (std::abs(lu_(pivot, k)) < 1e-300) {
      throw std::domain_error("LU: matrix is singular");
    }
    if (pivot != k) {
      std::swap_ranges(lu_.row(k).begin(), lu_.row(k).end(),
                       lu_.row(pivot).begin());
      std::swap(perm_[k], perm_[pivot]);
      sign_ = -sign_;
    }
    for (std::size_t i = k + 1; i < n; ++i) {
      const double factor = lu_(i, k) / lu_(k, k);
      lu_(i, k) = factor;
      for (std::size_t j = k + 1; j < n; ++j) lu_(i, j) -= factor * lu_(k, j);
    }
  }
}

std::vector<double> LuDecomposition::Solve(std::span<const double> b) const {
  const std::size_t n = lu_.rows();
  if (b.size() != n) throw std::invalid_argument("LU::Solve: size mismatch");
  std::vector<double> x(n);
  for (std::size_t i = 0; i < n; ++i) {  // forward: L y = P b
    double s = b[perm_[i]];
    for (std::size_t j = 0; j < i; ++j) s -= lu_(i, j) * x[j];
    x[i] = s;
  }
  for (std::size_t i = n; i-- > 0;) {  // backward: U x = y
    double s = x[i];
    for (std::size_t j = i + 1; j < n; ++j) s -= lu_(i, j) * x[j];
    x[i] = s / lu_(i, i);
  }
  return x;
}

double LuDecomposition::Determinant() const {
  double det = sign_;
  for (std::size_t i = 0; i < lu_.rows(); ++i) det *= lu_(i, i);
  return det;
}

Matrix LuDecomposition::Inverse() const {
  const std::size_t n = lu_.rows();
  Matrix inv(n, n);
  std::vector<double> e(n);
  for (std::size_t j = 0; j < n; ++j) {
    std::ranges::fill(e, 0.0);
    e[j] = 1.0;
    const auto col = Solve(e);
    for (std::size_t i = 0; i < n; ++i) inv(i, j) = col[i];
  }
  return inv;
}

std::vector<double> Correlate(const Matrix& cholesky_lower,
                              std::span<const double> z) {
  const std::size_t n = cholesky_lower.rows();
  if (z.size() != n) throw std::invalid_argument("Correlate: size mismatch");
  std::vector<double> y(n, 0.0);
  for (std::size_t i = 0; i < n; ++i) {
    const auto r = cholesky_lower.row(i);
    // Lower-triangular: only the first i+1 entries are non-zero.
    y[i] = std::inner_product(r.begin(),
                              r.begin() + static_cast<std::ptrdiff_t>(i + 1),
                              z.begin(), 0.0);
  }
  return y;
}

}  // namespace cpp4q::hanson::ch08
