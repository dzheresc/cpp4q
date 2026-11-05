// Chapter 1 demo: a call by Monte Carlo against Black-Scholes.

#include <initializer_list>
#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch01_simple_monte_carlo/simple_mc.h"

int main() {
  namespace j = cpp4q::joshi;
  const double expiry = 1, strike = 100, spot = 100, vol = 0.2, r = 0.05;
  j::Rng rng(2008);
  std::printf("Black-Scholes: %.6f\n",
              j::BlackScholesCall(spot, strike, r, 0, vol, expiry));
  for (const std::uint64_t n : {1'000ULL, 10'000ULL, 100'000ULL, 1'000'000ULL}) {
    const j::McResult mc =
        j::ch01::SimpleMonteCarlo1(expiry, strike, spot, vol, r, n, rng);
    std::printf("%8llu paths: %.6f +- %.6f\n",
                static_cast<unsigned long long>(n), mc.price, mc.std_error);
  }
}
