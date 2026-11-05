// Appendix A demo: a small table of Black-Scholes prices and Greeks.

#include <initializer_list>
#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"

int main() {
  namespace j = cpp4q::joshi;
  const double spot = 100, r = 0.05, d = 0.02, vol = 0.2, t = 1;
  std::printf("%6s %9s %9s %9s %9s %8s %8s %8s\n", "K", "call", "put",
              "dig call", "dig put", "delta", "gamma", "vega");
  for (const double k : {80.0, 90.0, 100.0, 110.0, 120.0}) {
    std::printf("%6.0f %9.4f %9.4f %9.4f %9.4f %8.4f %8.4f %8.4f\n", k,
                j::BlackScholesCall(spot, k, r, d, vol, t),
                j::BlackScholesPut(spot, k, r, d, vol, t),
                j::BlackScholesDigitalCall(spot, k, r, d, vol, t),
                j::BlackScholesDigitalPut(spot, k, r, d, vol, t),
                j::BlackScholesCallDelta(spot, k, r, d, vol, t),
                j::BlackScholesCallGamma(spot, k, r, d, vol, t),
                j::BlackScholesCallVega(spot, k, r, d, vol, t));
  }
}
