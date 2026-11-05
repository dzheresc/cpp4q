// Chapter 19 demo: one integral four ways, and an implied-volatility smile
// recovered from prices.

#include <cstdio>
#include <functional>
#include <initializer_list>

#include "ch03_functions/matlib.h"
#include "ch04_flow_of_control/control.h"
#include "ch19_function_objects/solvers.h"

namespace a = cpp4q::armstrong;

namespace {
double Pdf(double x) { return a::ch03::NormPdf(x); }
}  // namespace

int main() {
  constexpr int kN = 1000;
  const auto lambda = [](double x) { return a::ch03::NormPdf(x); };
  std::printf("integral of phi over [-3, 3] (exact %.12f):\n",
              a::ch03::NormCdf(3) - a::ch03::NormCdf(-3));
  std::printf("  function pointer %.12f\n",
              a::ch04::IntegrateMidpoint(Pdf, -3, 3, kN));
  std::printf("  virtual          %.12f\n",
              a::ch10::Integrate(a::ch19::FunctionAdapter(lambda), -3, 3, kN));
  std::printf("  std::function    %.12f\n",
              a::ch19::IntegrateStdFunction(lambda, -3, 3, kN));
  std::printf("  template         %.12f\n",
              a::ch19::IntegrateTemplate(lambda, -3, 3, kN));

  std::printf("\n%8s %10s %12s\n", "strike", "price", "implied vol");
  for (double k : {70.0, 90.0, 100.0, 110.0, 140.0}) {
    const double vol = 0.2 + 0.1 * std::abs(k - 100) / 100;  // a toy smile
    const double price = a::ch03::BlackScholesCallPrice(k, 1, 100, vol, 0.03);
    std::printf("%8.1f %10.5f %12.8f\n", k, price,
                a::ch19::ImpliedVolatility(price, k, 1, 100, 0.03));
  }
  return 0;
}
