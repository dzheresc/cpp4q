// Chapter 1 demo: hello concurrency, swap edition.

#include <cstdio>
#include <thread>

#include "ch01_hello_concurrency/swap_pricing.h"

int main() {
  namespace ch01 = cpp4q::williams::ch01;

  std::thread hello([] { std::puts("Hello from a second thread"); });
  hello.join();

  const auto fixed = ch01::MakeFixedLeg(1e6, 0.04, 5);
  // A floating leg that resets at 3.5% flat, for illustration.
  const auto floating = ch01::MakeFixedLeg(1e6, 0.035, 5);
  const auto v = ch01::PriceSwapConcurrently(fixed, floating, 0.035);
  std::printf("fixed PV %.2f, floating PV %.2f, NPV %.2f (%u hw threads)\n",
              v.fixed_leg_pv, v.floating_leg_pv, v.Npv(),
              ch01::HardwareThreads());
}
