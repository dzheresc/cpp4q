// Chapter 1 demo: plain Monte Carlo against Black-Scholes, the n^(-1/2)
// rate, and a first path-dependent payoff.

#include <cstdio>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/monte_carlo.h"

namespace g = cpp4q::glasserman::ch01;

int main() {
  const g::BlackScholesInputs in{};
  const double exact = g::BlackScholesPrice(g::OptionType::kCall, in);
  const g::Estimate e = g::PriceEuropeanCall(in, 200'000, 42);
  std::printf("ATM call: Black-Scholes %.4f, MC %.4f +- %.4f (95%%)\n", exact,
              e.mean, e.HalfWidth());

  std::printf("\n%10s %12s %12s\n", "paths", "|error|", "std error");
  const auto study = g::ConvergenceStudy(in, 8, 18, 7);
  for (const auto& p : study) {
    std::printf("%10zu %12.5f %12.5f\n", p.paths, p.abs_error, p.std_error);
  }
  std::printf("slope of log(std error) vs log(n): %.3f (theory -0.5)\n",
              g::StdErrorSlope(study));

  const g::Estimate asian = g::PriceArithmeticAsianCall(in, 12, 100'000, 1);
  std::printf("\nArithmetic Asian call (12 fixings): %.4f +- %.4f\n",
              asian.mean, asian.HalfWidth());
  std::printf("Geometric Asian call (closed form):  %.4f\n",
              g::GeometricAsianCallPrice(in, 12));
  return 0;
}
