// Chapter 13 demo: invalid inputs reported through a no-throw boundary, and
// a bump that is undone even when the pricer throws.

#include <initializer_list>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "appendix_a_black_scholes/black_scholes.h"
#include "ch13_exceptions/exceptions.h"

int main() {
  namespace j = cpp4q::joshi;
  namespace c13 = j::ch13;
  for (const double vol : {0.2, -0.1}) {
    std::string message;
    double price = 0.0;
    const c13::Status status = c13::CallNoThrow(
        [&] {
          c13::ValidateBsInputs(100, 100, vol, 1);
          price = j::BlackScholesCall(100, 100, 0.05, 0, vol, 1);
        },
        &message);
    std::printf("vol %5.2f: status %d price %.4f %s\n", vol,
                static_cast<int>(status), price, message.c_str());
  }

  c13::Market market;
  const auto bs = [](const c13::Market& m) {
    return j::BlackScholesCall(m.spot, 100, m.r, 0, m.vol, 1);
  };
  std::printf("delta %.6f, spot after bump %.1f\n",
              c13::BumpedDelta(market, 0.01, bs), market.spot);
  try {
    c13::BumpedDelta(market, 0.01, [](const c13::Market&) -> double {
      throw std::runtime_error("pricer failed");
    });
  } catch (const std::exception& e) {
    std::printf("caught '%s', spot still %.1f\n", e.what(), market.spot);
  }
}
