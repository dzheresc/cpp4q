// Chapter 9 demo: implied volatility by bisection and by Newton-Raphson.

#include <initializer_list>
#include <cstdio>

#include "ch09_solvers_implied_vol/solvers.h"

int main() {
  namespace c9 = cpp4q::joshi::ch09;
  const double r = 0.05, d = 0.01, t = 0.5, spot = 100;
  std::printf("%6s %8s %14s %14s\n", "strike", "price", "bisection",
              "newton");
  for (const double k : {70.0, 90.0, 100.0, 110.0, 140.0}) {
    const c9::BsCall call(r, d, t, spot, k);
    const double price = call.Price(0.27);
    std::printf("%6.0f %8.4f %14.10f %14.10f\n", k, price,
                c9::ImpliedVolBisection(call, price),
                c9::ImpliedVolNewton(call, price));
  }
}
