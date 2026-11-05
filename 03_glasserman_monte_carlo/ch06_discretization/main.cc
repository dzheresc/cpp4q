// Chapter 6 demo: strong and weak convergence of Euler and Milstein,
// Richardson extrapolation, and Euler fixes for CIR.

#include <cmath>
#include <cstdio>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch06_discretization/schemes.h"

namespace g = cpp4q::glasserman;
using g::ch06::Scheme;

int main() {
  const g::ch03::GbmParams gbm{100.0, 0.05, 0.4};
  const std::vector<std::size_t> steps = {4, 8, 16, 32, 64, 128};
  std::vector<double> euler, milstein;
  std::printf("%6s %14s %14s\n", "steps", "Euler E|err|", "Milstein E|err|");
  for (std::size_t n : steps) {
    euler.push_back(
        g::ch06::GbmStrongError(gbm, 1.0, n, Scheme::kEuler, 20'000, 1).mean);
    milstein.push_back(
        g::ch06::GbmStrongError(gbm, 1.0, n, Scheme::kMilstein, 20'000, 1)
            .mean);
    std::printf("%6zu %14.6f %14.6f\n", n, euler.back(), milstein.back());
  }
  std::printf("strong order: Euler %.3f (0.5), Milstein %.3f (1.0)\n",
              g::ch06::FitOrder(steps, euler),
              g::ch06::FitOrder(steps, milstein));

  std::printf("\nWeak error of Euler E[S(T)^2] (exact moments):\n");
  const double exact2 = 100.0 * 100.0 * std::exp((2 * 0.05 + 0.16) * 1.0);
  for (std::size_t n : {1u, 2u, 4u, 8u}) {
    const double e1 = g::ch06::EulerGbmSecondMoment(gbm, 1.0, n) - exact2;
    const double e2 = 2.0 * g::ch06::EulerGbmSecondMoment(gbm, 1.0, 2 * n) -
                      g::ch06::EulerGbmSecondMoment(gbm, 1.0, n) - exact2;
    std::printf("  steps %2zu: Euler %10.4f   Richardson %10.4f\n", n, e1, e2);
  }

  const g::ch01::BlackScholesInputs in{.vol = 0.4};
  const double bs = g::ch01::BlackScholesPrice(g::ch01::OptionType::kCall, in);
  std::printf("\nCall, vol 40%%: Black-Scholes %.4f\n", bs);
  for (std::size_t n : {1u, 2u, 4u}) {
    const auto e = g::ch06::EulerCallPrice(in, n, 2'000'000, false, 3);
    const auto rich = g::ch06::RichardsonCallPrice(in, n, 2'000'000, 3);
    std::printf(
        "  %zu step(s): Euler %.4f +- %.4f, Richardson (%zu/%zu) "
        "%.4f +- %.4f\n",
        n, e.mean, e.HalfWidth(), n, 2 * n, rich.mean, rich.HalfWidth());
  }

  const g::ch03::ShortRateParams cir{.kappa = 0.2, .theta = 0.04, .sigma = 0.3};
  std::printf(
      "\nCIR 5y ZCB, Feller violated (2 kappa theta < sigma^2): "
      "exact %.5f\n",
      g::ch03::CirBondPrice(cir, 0.02, 5.0));
  const char* names[] = {"absorb", "reflect", "full truncation"};
  int i = 0;
  for (auto fix : {g::ch06::CirFix::kAbsorb, g::ch06::CirFix::kReflect,
                   g::ch06::CirFix::kFullTruncation}) {
    const auto e = g::ch06::CirEulerBond(cir, 0.02, 5.0, 20, fix, 200'000, 9);
    std::printf("  %-16s %.5f +- %.5f\n", names[i++], e.mean, e.HalfWidth());
  }
  return 0;
}
