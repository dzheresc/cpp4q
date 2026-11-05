// Chapter 4 demo: copying options through the bridge, and a term structure
// of volatility that prices like its root-mean-square constant.

#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch04_virtual_constructor/vanilla.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c4 = j::ch04;
  const c4::VanillaOption call(c4::PayOffCall(100), 1.0);
  const c4::VanillaOption copy = call;  // clones the pay-off
  std::printf("payoff(120): original %g, copy %g\n", call.OptionPayOff(120),
              copy.OptionPayOff(120));

  const c4::ParametersPiecewiseConstant term_vol({0.25, 0.5}, {0.3, 0.2, 0.1});
  const c4::Parameters vol = term_vol;
  const c4::Parameters r = 0.05;
  const double rms = vol.RootMeanSquare(0, 1);
  j::Rng rng(4);
  const auto mc = c4::SimpleMonteCarlo4(copy, 100, vol, r, 400'000, rng);
  std::printf("term-structure vol: rms %.6f\n", rms);
  std::printf("MC %.4f +- %.4f, BS(rms) %.4f\n", mc.price, mc.std_error,
              j::BlackScholesCall(100, 100, 0.05, 0, rms, 1));
}
