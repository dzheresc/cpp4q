// Chapter 11 demo: a stress test that exposes a check-then-act race in a
// pre-trade risk limit, and passes on the fixed version.

#include <cstdio>

#include "ch11_testing_debugging/stress_harness.h"

int main() {
  namespace ch11 = cpp4q::williams::ch11;
  ch11::PreTradeLimit limit(1'000);

  auto run = [&](bool naive) {
    return ch11::CountFailingRounds(
        200, 8, [&] { limit.Reset(); },
        [&](unsigned) {
          for (int i = 0; i < 50; ++i) {
            if (naive) {
              (void)limit.TryReserveNaive(7);
            } else {
              (void)limit.TryReserve(7);
            }
          }
        },
        [&] { return limit.used() <= limit.limit(); });
  };
  std::printf("naive check-then-act: limit breached in %d / 200 rounds\n",
              run(true));
  std::printf("CAS reserve:          limit breached in %d / 200 rounds\n",
              run(false));
}
