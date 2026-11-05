// Chapter 1: Hello, world of concurrency in C++!
//
// The smallest useful concurrent program: value the two legs of an interest
// rate swap at the same time, one on a std::thread and one on the calling
// thread, then join and combine.

#ifndef CPP4Q_WILLIAMS_CH01_HELLO_CONCURRENCY_SWAP_PRICING_H_
#define CPP4Q_WILLIAMS_CH01_HELLO_CONCURRENCY_SWAP_PRICING_H_

#include <vector>

namespace cpp4q::williams::ch01 {

struct CashFlow {
  double time = 0.0;    // year fraction from today
  double amount = 0.0;  // signed, in currency units
};

using Leg = std::vector<CashFlow>;

struct SwapValuation {
  double fixed_leg_pv = 0.0;
  double floating_leg_pv = 0.0;
  // Receiver-fixed NPV.
  [[nodiscard]] double Npv() const { return fixed_leg_pv - floating_leg_pv; }
};

// Present value under a flat continuously-compounded rate.
[[nodiscard]] double LegPv(const Leg& leg, double rate);

// Same result as valuing both legs one after the other; the floating leg is
// valued on a second thread.
[[nodiscard]] SwapValuation PriceSwapConcurrently(const Leg& fixed,
                                                  const Leg& floating,
                                                  double rate);

// Quarterly fixed leg on `notional` for `years`, plus the notional exchange.
[[nodiscard]] Leg MakeFixedLeg(double notional, double coupon_rate, int years);

// std::thread::hardware_concurrency(), or 1 when the runtime can't tell.
[[nodiscard]] unsigned HardwareThreads();

}  // namespace cpp4q::williams::ch01

#endif  // CPP4Q_WILLIAMS_CH01_HELLO_CONCURRENCY_SWAP_PRICING_H_
