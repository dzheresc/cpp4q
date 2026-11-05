// Chapter 4 demo: variance ratios of each technique at equal sample size.

#include <cstdio>

#include "ch01_foundations/black_scholes.h"
#include "ch02_random_numbers/normal.h"
#include "ch04_variance_reduction/variance_reduction.h"

namespace g = cpp4q::glasserman;
using g::ch01::Estimate;

namespace {

void Row(const char* name, const Estimate& e, const Estimate& base) {
  // Variance per draw, base over technique: the factor by which the
  // technique cuts the number of draws for a given accuracy.
  const double ratio =
      (base.std_error * base.std_error * static_cast<double>(base.samples)) /
      (e.std_error * e.std_error * static_cast<double>(e.samples));
  std::printf("  %-28s %9.4f +- %7.4f   variance ratio %8.1f\n", name, e.mean,
              e.HalfWidth(), ratio);
}

}  // namespace

int main() {
  g::ch02::Sampler s(g::ch02::Mrg32k3a(11));
  const g::ch01::BlackScholesInputs atm{};
  std::printf("ATM call, Black-Scholes %.4f\n",
              g::ch01::BlackScholesPrice(g::ch01::OptionType::kCall, atm));
  const auto plain = g::ch04::PlainCall(atm, 100'000, s);
  Row("plain", plain, plain);
  // A pair uses two payoff evaluations: count both.
  auto anti = g::ch04::AntitheticCall(atm, 50'000, s);
  anti.samples *= 2;
  Row("antithetic (per draw)", anti, plain);
  Row("control variate S(T)",
      g::ch04::ControlVariateCall(atm, 100'000, s).estimate, plain);
  Row("stratified (100 strata)", g::ch04::StratifiedCall(atm, 100, 1000, s),
      plain);

  g::ch01::BlackScholesInputs otm = atm;
  otm.strike = 160.0;
  std::printf("\nOTM call K=160, Black-Scholes %.5f\n",
              g::ch01::BlackScholesPrice(g::ch01::OptionType::kCall, otm));
  const auto otm_plain = g::ch04::PlainCall(otm, 100'000, s);
  Row("plain", otm_plain, otm_plain);
  Row("importance sampling",
      g::ch04::ImportanceSamplingCall(otm, g::ch04::StrikeCenteringShift(otm),
                                      100'000, s),
      otm_plain);

  const g::ch04::AsianSpec asian{atm, 12};
  std::printf("\nArithmetic Asian, 12 fixings (geometric closed form %.4f)\n",
              g::ch01::GeometricAsianCallPrice(atm, 12));
  const auto a_plain = g::ch04::PlainAsian(asian, 100'000, s);
  Row("plain", a_plain, a_plain);
  const auto cv = g::ch04::GeometricControlAsian(asian, 100'000, s);
  Row("geometric control", cv.estimate, a_plain);
  std::printf("  (beta %.4f, correlation %.5f)\n", cv.beta, cv.correlation);
  Row("terminal stratification",
      g::ch04::TerminalStratifiedAsian(asian, 100, 1000, s), a_plain);
  return 0;
}
