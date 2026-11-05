// Chapter 16 demo: matrix algebra, Cholesky, and correlated stocks.

#include <cstdio>
#include <iostream>
#include <vector>

#include "ch16_matrix/matrix.h"
#include "ch16_matrix/multi_stock.h"

namespace a = cpp4q::armstrong::ch16;

int main() {
  const a::Matrix rho{{1.0, 0.5, 0.2}, {0.5, 1.0, 0.3}, {0.2, 0.3, 1.0}};
  const a::Matrix l = a::Cholesky(rho);
  std::cout << "correlation:\n" << rho << "Cholesky factor L:\n" << l
            << "max |L L^T - rho| = "
            << a::MaxAbsDifference(l * l.Transpose(), rho) << "\n";

  const std::vector<double> b{1.0, 2.0, 3.0};
  const auto x = a::Solve(rho, b);
  std::printf("solve rho x = (1,2,3): x = (%.6f, %.6f, %.6f)\n", x[0], x[1],
              x[2]);

  const a::MultiStockModel model({100, 95, 110}, {0.2, 0.3, 0.25}, rho, 0.05);
  a::Rng rng(16);
  const auto ex = a::PriceExchangeOption(model, 1.0, 200'000, rng);
  std::printf("\nexchange option S0 for S1: MC %.4f +- %.4f, Margrabe %.4f\n",
              ex.price, ex.std_error,
              a::MargrabePrice(100, 95, 0.2, 0.3, 0.5, 1.0));
  const std::vector<double> w{1.0 / 3, 1.0 / 3, 1.0 / 3};
  const auto basket = a::PriceBasketCall(model, w, 100, 1.0, 200'000, rng);
  std::printf("equal-weight basket call K=100: %.4f +- %.4f\n", basket.price,
              basket.std_error);
  return 0;
}
