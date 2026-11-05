// Chapter 21 demo: the PDE pricer against Black-Scholes, and the early
// exercise premium of an American put.

#include <cstdio>
#include <initializer_list>

#include "ch03_functions/matlib.h"
#include "ch21_next_steps/pde.h"

namespace a = cpp4q::armstrong;
using a::ch21::ExerciseStyle;
using a::ch21::PayoffType;

int main() {
  std::printf("%6s %12s %12s %12s %12s\n", "grid", "PDE call", "BS call",
              "PDE delta", "BS delta");
  for (int n : {50, 100, 200, 400, 800}) {
    const a::ch21::PdeSettings s{.n_space = n, .n_time = n};
    const auto r = a::ch21::PricePde(PayoffType::kCall,
                                     ExerciseStyle::kEuropean, 100, 1, 100,
                                     0.2, 0.05, s);
    std::printf("%6d %12.7f %12.7f %12.7f %12.7f\n", n, r.price,
                a::ch03::BlackScholesCallPrice(100, 1, 100, 0.2, 0.05),
                r.delta,
                a::ch03::BlackScholesCallDelta(100, 1, 100, 0.2, 0.05));
  }
  std::printf("\n%8s %12s %12s %12s\n", "strike", "European", "American",
              "premium");
  for (double k : {90.0, 100.0, 110.0, 120.0}) {
    const double eu = a::ch21::PricePde(PayoffType::kPut,
                                        ExerciseStyle::kEuropean, k, 1, 100,
                                        0.2, 0.05)
                          .price;
    const double am = a::ch21::PricePde(PayoffType::kPut,
                                        ExerciseStyle::kAmerican, k, 1, 100,
                                        0.2, 0.05)
                          .price;
    std::printf("%8.1f %12.6f %12.6f %12.6f\n", k, eu, am, am - eu);
  }
  return 0;
}
