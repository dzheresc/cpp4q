// Chapter 2 demo: one Monte Carlo routine, two pay-offs.

#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch02_encapsulation/payoff1.h"

int main() {
  namespace j = cpp4q::joshi;
  const double expiry = 1, strike = 100, spot = 100, vol = 0.2, r = 0.05;
  j::Rng rng(2);
  const j::ch02::PayOff call(strike, j::ch02::OptionType::kCall);
  const j::ch02::PayOff put(strike, j::ch02::OptionType::kPut);
  const auto c = j::ch02::SimpleMonteCarlo2(call, expiry, spot, vol, r,
                                            200'000, rng);
  const auto p = j::ch02::SimpleMonteCarlo2(put, expiry, spot, vol, r,
                                            200'000, rng);
  std::printf("call %.4f +- %.4f (BS %.4f)\n", c.price, c.std_error,
              j::BlackScholesCall(spot, strike, r, 0, vol, expiry));
  std::printf("put  %.4f +- %.4f (BS %.4f)\n", p.price, p.std_error,
              j::BlackScholesPut(spot, strike, r, 0, vol, expiry));
}
