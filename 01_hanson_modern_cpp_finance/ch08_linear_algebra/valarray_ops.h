// Appendix E: std::valarray and slice-based matrix operations.
//
// valarray gives element-wise arithmetic and std::slice / std::gslice views
// over a flat buffer, which is enough to express rows, columns and a
// matrix product without a matrix class.

#ifndef CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_VALARRAY_OPS_H_
#define CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_VALARRAY_OPS_H_

#include <cstddef>
#include <valarray>

namespace cpp4q::hanson::ch08 {

struct ValMatrix {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::valarray<double> data;  // row-major
};

std::valarray<double> Row(const ValMatrix& m, std::size_t i);
std::valarray<double> Col(const ValMatrix& m, std::size_t j);
ValMatrix MultiplyValarray(const ValMatrix& a, const ValMatrix& b);

// Element-wise GBM step on a whole vector of spots in one expression.
std::valarray<double> GbmStep(const std::valarray<double>& spots,
                              const std::valarray<double>& normals,
                              double drift_dt, double vol_sqrt_dt);

}  // namespace cpp4q::hanson::ch08

#endif  // CPP4Q_HANSON_CH08_LINEAR_ALGEBRA_VALARRAY_OPS_H_
