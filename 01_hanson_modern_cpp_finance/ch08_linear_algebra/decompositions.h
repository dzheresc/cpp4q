// Chapter 8: Cholesky and LU, plus their main quant use - turning
// independent normals into correlated ones.

#ifndef CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_DECOMPOSITIONS_H_
#define CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_DECOMPOSITIONS_H_

#include <span>
#include <vector>

#include "ch08_linear_algebra/matrix.h"

namespace cpp4q::hanson::ch08 {

// Lower-triangular L with A = L L^T. Throws std::domain_error if A is not
// symmetric positive definite.
Matrix Cholesky(const Matrix& a);

// PA = LU with partial pivoting, stored compactly.
class LuDecomposition {
 public:
  // Throws std::domain_error if A is singular (to working precision).
  explicit LuDecomposition(Matrix a);
  [[nodiscard]] std::vector<double> Solve(std::span<const double> b) const;
  [[nodiscard]] double Determinant() const;
  [[nodiscard]] Matrix Inverse() const;

 private:
  Matrix lu_;
  std::vector<std::size_t> perm_;
  int sign_ = 1;
};

// y = L z: correlate a vector of independent standard normals.
std::vector<double> Correlate(const Matrix& cholesky_lower,
                              std::span<const double> z);

}  // namespace cpp4q::hanson::ch08

#endif  // CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_DECOMPOSITIONS_H_
