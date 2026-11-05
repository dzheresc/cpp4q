// Appendix A demo: one-step replication, the martingale property, and CRR
// converging to Black-Scholes.

#include <cstdio>
#include <initializer_list>

#include "appendix_a_risk_neutral/binomial.h"
#include "ch03_functions/matlib.h"

namespace a = cpp4q::armstrong;
using a::appendix_a::ExerciseStyle;
using a::appendix_a::PayoffType;

int main() {
  // One step: S = 100 goes to 120 or 90; a call struck at 100.
  const auto r = a::appendix_a::ReplicateOneStep(100, 1.2, 0.9, 0.05, 1, 20, 0);
  std::printf("replicate call K=100 (S 100 -> 120 / 90, r = 5%%): "
              "%.4f shares + %.4f bond = %.4f\n",
              r.shares, r.bond, r.price);
  std::printf("risk-neutral q = %.6f; E_q[e^-rT S_T] on a 500-step tree = "
              "%.12f\n",
              a::appendix_a::RiskNeutralProbability(1.2, 0.9, 0.05, 1),
              a::appendix_a::CrrExpectation([](double s) { return s; }, 1, 100,
                                            0.2, 0.05, 500));
  const double bs = a::ch03::BlackScholesCallPrice(100, 1, 100, 0.2, 0.05);
  std::printf("\n%8s %14s %12s %14s\n", "steps", "CRR call", "error",
              "American put");
  for (int n : {10, 50, 100, 500, 1000}) {
    const double c = a::appendix_a::CrrPrice(
        PayoffType::kCall, ExerciseStyle::kEuropean, 100, 1, 100, 0.2, 0.05, n);
    std::printf("%8d %14.8f %12.2e %14.8f\n", n, c, c - bs,
                a::appendix_a::CrrPrice(PayoffType::kPut,
                                        ExerciseStyle::kAmerican, 100, 1, 100,
                                        0.2, 0.05, n));
  }
  return 0;
}
