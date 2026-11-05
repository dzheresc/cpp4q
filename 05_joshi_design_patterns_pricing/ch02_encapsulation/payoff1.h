// Chapter 2: Encapsulation.
//
// The pay-off becomes a class, so the Monte Carlo routine no longer knows
// what it is pricing: it only evaluates payoff(S). This first design picks
// the contract with an enum and a switch, which is encapsulated but not
// closed for modification: adding a digital means editing PayOff itself
// and every switch over OptionType. Chapter 3 fixes that with inheritance.

#ifndef CPP4Q_JOSHI_CH02_ENCAPSULATION_PAYOFF1_H_
#define CPP4Q_JOSHI_CH02_ENCAPSULATION_PAYOFF1_H_

#include <cstdint>

#include "ch01_simple_monte_carlo/simple_mc.h"

namespace cpp4q::joshi::ch02 {

enum class OptionType { kCall, kPut };

class PayOff {
 public:
  PayOff(double strike, OptionType type) : strike_(strike), type_(type) {}

  double operator()(double spot) const;
  double strike() const { return strike_; }
  OptionType type() const { return type_; }

 private:
  double strike_;
  OptionType type_;
};

McResult SimpleMonteCarlo2(const PayOff& payoff, double expiry, double spot,
                           double vol, double r, std::uint64_t num_paths,
                           Rng& rng);

}  // namespace cpp4q::joshi::ch02

#endif  // CPP4Q_JOSHI_CH02_ENCAPSULATION_PAYOFF1_H_
