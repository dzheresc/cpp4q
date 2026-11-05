#include "ch01_hello_concurrency/swap_pricing.h"

#include <cmath>
#include <thread>

namespace cpp4q::williams::ch01 {

double LegPv(const Leg& leg, double rate) {
  double pv = 0.0;
  for (const auto& cf : leg) pv += cf.amount * std::exp(-rate * cf.time);
  return pv;
}

SwapValuation PriceSwapConcurrently(const Leg& fixed, const Leg& floating,
                                    double rate) {
  SwapValuation result;
  // The worker writes only `floating_leg_pv`; join() orders that write
  // before the read below, so no further synchronisation is needed.
  std::thread worker([&] { result.floating_leg_pv = LegPv(floating, rate); });
  result.fixed_leg_pv = LegPv(fixed, rate);
  worker.join();
  return result;
}

Leg MakeFixedLeg(double notional, double coupon_rate, int years) {
  Leg leg;
  const int periods = 4 * years;
  leg.reserve(static_cast<std::size_t>(periods) + 1);
  for (int i = 1; i <= periods; ++i) {
    leg.push_back({.time = 0.25 * i, .amount = notional * coupon_rate * 0.25});
  }
  leg.push_back({.time = static_cast<double>(years), .amount = notional});
  return leg;
}

unsigned HardwareThreads() {
  const unsigned n = std::thread::hardware_concurrency();
  return n == 0 ? 1 : n;
}

}  // namespace cpp4q::williams::ch01
