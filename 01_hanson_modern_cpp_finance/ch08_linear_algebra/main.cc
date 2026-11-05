// Chapter 8 demo: products, Cholesky, LU solve, correlated normals.

#include <iostream>
#include <vector>

#include "ch08_linear_algebra/decompositions.h"
#include "ch08_linear_algebra/matrix.h"
#include "ch08_linear_algebra/valarray_ops.h"

int main() {
  using namespace cpp4q::hanson::ch08;  // NOLINT(build/namespaces) demo only

  const Matrix corr{{1.0, 0.6, 0.3}, {0.6, 1.0, 0.5}, {0.3, 0.5, 1.0}};
  const Matrix l = Cholesky(corr);
  std::cout << "L =\n"
            << l << "L L^T - C max err: " << MaxAbsDiff(l * l.Transpose(), corr)
            << '\n';

  const LuDecomposition lu(corr);
  const std::vector<double> b{1.0, 2.0, 3.0};
  const auto x = lu.Solve(b);
  std::cout << "solve C x = b: " << x[0] << ' ' << x[1] << ' ' << x[2]
            << "  det " << lu.Determinant() << '\n';

  const auto y = Correlate(l, std::vector{0.5, -1.0, 0.25});
  std::cout << "correlated: " << y[0] << ' ' << y[1] << ' ' << y[2] << '\n';

  const ValMatrix va{2, 2, {1.0, 2.0, 3.0, 4.0}};
  const auto vc = MultiplyValarray(va, va);
  std::cout << "valarray A*A = [" << vc.data[0] << ' ' << vc.data[1] << "; "
            << vc.data[2] << ' ' << vc.data[3] << "]\n";
}
