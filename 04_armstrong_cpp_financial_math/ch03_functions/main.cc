// Chapter 3 demo: the matlib functions and a Black-Scholes table.

#include <cstdio>
#include <initializer_list>

#include "ch03_functions/matlib.h"

namespace a = cpp4q::armstrong::ch03;

int main() {
  std::printf("%6s %22s %22s %12s\n", "x", "NormCdf", "A&S 26.2.17", "error");
  for (double x : {-3.0, -1.0, 0.0, 0.5, 1.96, 4.0}) {
    std::printf("%6.2f %22.16f %22.16f %12.2e\n", x, a::NormCdf(x),
                a::NormCdfPolynomial(x),
                a::NormCdfPolynomial(x) - a::NormCdf(x));
  }
  std::printf("\nNormInv(0.975) = %.15f\n", a::NormInv(0.975));

  std::printf("\n%8s %10s %10s %10s\n", "strike", "call", "put", "delta");
  for (double k : {80.0, 90.0, 100.0, 110.0, 120.0}) {
    std::printf("%8.1f %10.4f %10.4f %10.4f\n", k,
                a::BlackScholesCallPrice(k, 1.0, 100.0, 0.2, 0.05),
                a::BlackScholesPutPrice(k, 1.0, 100.0, 0.2, 0.05),
                a::BlackScholesCallDelta(k, 1.0, 100.0, 0.2, 0.05));
  }
  if (const auto roots = a::SolveQuadratic(1.0, -1e8, 1.0)) {
    std::printf("\nroots of x^2 - 1e8 x + 1: %.17g, %.17g\n", roots->first,
                roots->second);
  }
  return 0;
}
