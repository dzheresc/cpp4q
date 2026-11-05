// Chapter 3 demo: pricing through the abstract base.

#include <cstdio>
#include <memory>
#include <vector>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch03_inheritance/payoff2.h"

int main() {
  namespace j = cpp4q::joshi;
  using j::ch03::PayOff;
  const double expiry = 1, spot = 100, vol = 0.2, r = 0.05;
  std::vector<std::unique_ptr<PayOff>> book;
  book.push_back(std::make_unique<j::ch03::PayOffCall>(100));
  book.push_back(std::make_unique<j::ch03::PayOffPut>(100));
  book.push_back(std::make_unique<j::ch03::PayOffDoubleDigital>(90, 110));
  const double exact[] = {
      j::BlackScholesCall(spot, 100, r, 0, vol, expiry),
      j::BlackScholesPut(spot, 100, r, 0, vol, expiry),
      j::BlackScholesDigitalCall(spot, 90, r, 0, vol, expiry) -
          j::BlackScholesDigitalCall(spot, 110, r, 0, vol, expiry)};
  const char* names[] = {"call", "put", "double digital"};
  j::Rng rng(3);
  for (std::size_t i = 0; i < book.size(); ++i) {
    const auto mc =
        j::ch03::SimpleMonteCarlo3(*book[i], expiry, spot, vol, r, 200'000, rng);
    std::printf("%-15s %.4f +- %.4f (exact %.4f)\n", names[i], mc.price,
                mc.std_error, exact[i]);
  }
}
