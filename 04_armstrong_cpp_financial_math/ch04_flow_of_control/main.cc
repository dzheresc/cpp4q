// Chapter 4 demo: loops and branches doing numerical work.

#include <cmath>
#include <cstdio>
#include <initializer_list>

#include "ch03_functions/matlib.h"
#include "ch04_flow_of_control/control.h"

namespace a = cpp4q::armstrong;

int main() {
  std::printf("20! = %llu\n",
              static_cast<unsigned long long>(a::ch04::Factorial(20)));
  std::printf("primes below 50:");
  for (auto p : a::ch04::PrimesUpTo(50)) {
    std::printf(" %llu", static_cast<unsigned long long>(p));
  }
  std::printf("\nexp(-20) by series %.17g vs std::exp %.17g\n",
              a::ch04::ExpBySeries(-20), std::exp(-20.0));
  std::printf("10-year annuity at 5%%: immediate %.6f, due %.6f\n",
              a::ch04::AnnuityPresentValue(a::ch04::AnnuityType::kImmediate,
                                           0.05, 10),
              a::ch04::AnnuityPresentValue(a::ch04::AnnuityType::kDue, 0.05,
                                           10));
  std::printf("root of cos(x) - x: %.15f\n",
              a::ch04::Bisection([](double x) { return std::cos(x) - x; }, 0,
                                 1));
  std::printf("\n%8s %18s %12s\n", "steps", "call by integral", "error");
  const double exact = a::ch03::BlackScholesCallPrice(100, 1, 100, 0.2, 0.05);
  for (int n : {10, 100, 1000, 10000}) {
    const double v = a::ch04::CallPriceByIntegration(100, 1, 100, 0.2, 0.05, n);
    std::printf("%8d %18.12f %12.2e\n", n, v, v - exact);
  }
  return 0;
}
