// Chapter 7 demo: finite differences with and without common random
// numbers; pathwise vs likelihood ratio Greeks against closed forms.

#include <cstdio>

#include "ch01_foundations/black_scholes.h"
#include "ch07_sensitivities/greeks.h"

namespace g = cpp4q::glasserman;
using g::ch01::Estimate;
using g::ch07::FiniteDifference;

namespace {

void Row(const char* name, const Estimate& e, double exact) {
  std::printf("  %-34s %9.5f +- %8.5f   (exact %.5f)\n", name, e.mean,
              e.HalfWidth(), exact);
}

}  // namespace

int main() {
  const g::ch01::BlackScholesInputs in{};
  const double delta =
      g::ch01::BlackScholesDelta(g::ch01::OptionType::kCall, in);
  std::printf("Call delta by finite differences, h = 1%% of spot:\n");
  Row("forward, independent draws",
      g::ch07::FiniteDifferenceDelta(in, 0.01, FiniteDifference::kForward,
                                     false, 100'000, 1),
      delta);
  Row("forward, common random numbers",
      g::ch07::FiniteDifferenceDelta(in, 0.01, FiniteDifference::kForward, true,
                                     100'000, 1),
      delta);
  Row("central, common random numbers",
      g::ch07::FiniteDifferenceDelta(in, 0.01, FiniteDifference::kCentral, true,
                                     100'000, 1),
      delta);

  const auto gk = g::ch07::EuropeanCallGreeks(in, 200'000, 2);
  std::printf("\nCall Greeks from one simulation:\n");
  Row("price", gk.price,
      g::ch01::BlackScholesPrice(g::ch01::OptionType::kCall, in));
  Row("delta, pathwise", gk.pathwise_delta, delta);
  Row("delta, likelihood ratio", gk.lr_delta, delta);
  Row("vega, pathwise", gk.pathwise_vega, g::ch01::BlackScholesVega(in));
  Row("vega, likelihood ratio", gk.lr_vega, g::ch01::BlackScholesVega(in));
  Row("gamma, likelihood ratio", gk.lr_gamma, g::ch01::BlackScholesGamma(in));

  const auto dg = g::ch07::DigitalCallGreeks(in, 200'000, 3);
  std::printf("\nDigital call:\n");
  Row("delta, pathwise (wrong)", dg.pathwise_delta,
      g::ch01::DigitalCallDelta(in));
  Row("delta, likelihood ratio", dg.lr_delta, g::ch01::DigitalCallDelta(in));

  const auto ad = g::ch07::AsianCallDelta(in, 12, 200'000, 4);
  std::printf("\nAsian call (12 fixings) delta, no closed form:\n");
  std::printf("  pathwise %.5f +- %.5f, likelihood ratio %.5f +- %.5f\n",
              ad.pathwise_delta.mean, ad.pathwise_delta.HalfWidth(),
              ad.lr_delta.mean, ad.lr_delta.HalfWidth());
  return 0;
}
