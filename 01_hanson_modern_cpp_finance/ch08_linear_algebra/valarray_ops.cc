#include "ch08_linear_algebra/valarray_ops.h"

#include <cmath>
#include <stdexcept>

namespace cpp4q::hanson::ch08 {

std::valarray<double> Row(const ValMatrix& m, std::size_t i) {
  return m.data[std::slice(i * m.cols, m.cols, 1)];
}

std::valarray<double> Col(const ValMatrix& m, std::size_t j) {
  return m.data[std::slice(j, m.rows, m.cols)];
}

ValMatrix MultiplyValarray(const ValMatrix& a, const ValMatrix& b) {
  if (a.cols != b.rows) throw std::invalid_argument("valarray: dims differ");
  ValMatrix c{a.rows, b.cols, std::valarray<double>(a.rows * b.cols)};
  for (std::size_t i = 0; i < a.rows; ++i) {
    const std::valarray<double> ai = Row(a, i);
    for (std::size_t j = 0; j < b.cols; ++j) {
      c.data[i * b.cols + j] = (ai * Col(b, j)).sum();
    }
  }
  return c;
}

std::valarray<double> GbmStep(const std::valarray<double>& spots,
                              const std::valarray<double>& normals,
                              double drift_dt, double vol_sqrt_dt) {
  return spots * std::exp(drift_dt + vol_sqrt_dt * normals);
}

}  // namespace cpp4q::hanson::ch08
