// Chapter 8 demo: European and American options on one tree.

#include <initializer_list>
#include <cstdio>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch08_trees/tree.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c4 = j::ch04;
  namespace c8 = j::ch08;
  const double spot = 100, strike = 100, r = 0.05, d = 0.02, vol = 0.2, t = 1;
  const c8::TreeEuropean euro_call(t, c4::PayOffCall(strike));
  const c8::TreeEuropean euro_put(t, c4::PayOffPut(strike));
  const c8::TreeAmerican amer_put(t, c4::PayOffPut(strike));
  std::printf("BS call %.6f, BS put %.6f\n",
              j::BlackScholesCall(spot, strike, r, d, vol, t),
              j::BlackScholesPut(spot, strike, r, d, vol, t));
  std::printf("%6s %10s %10s %10s\n", "steps", "euro call", "euro put",
              "amer put");
  for (const std::size_t steps : {10u, 100u, 1000u}) {
    c8::SimpleBinomialTree tree(spot, r, d, vol, steps, t);
    std::printf("%6zu %10.6f %10.6f %10.6f\n", steps,
                tree.GetThePrice(euro_call), tree.GetThePrice(euro_put),
                tree.GetThePrice(amer_put));
  }
}
