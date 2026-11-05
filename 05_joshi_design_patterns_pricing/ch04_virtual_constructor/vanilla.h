// Chapter 4: the vanilla option and the Monte Carlo routine that prices it
// with time-dependent volatility and rates.

#ifndef CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_VANILLA_H_
#define CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_VANILLA_H_

#include <cstdint>

#include "ch01_simple_monte_carlo/simple_mc.h"
#include "ch04_virtual_constructor/parameters.h"
#include "ch04_virtual_constructor/payoff_bridge.h"

namespace cpp4q::joshi::ch04 {

// Copyable: the compiler-generated copy goes through PayOffBridge.
class VanillaOption {
 public:
  VanillaOption(PayOffBridge payoff, double expiry)
      : payoff_(std::move(payoff)), expiry_(expiry) {}

  double expiry() const { return expiry_; }
  double OptionPayOff(double spot) const { return payoff_(spot); }

 private:
  PayOffBridge payoff_;
  double expiry_;
};

// S(T) = S0 exp(int r - int sigma^2 / 2 + sqrt(int sigma^2) Z).
McResult SimpleMonteCarlo4(const VanillaOption& option, double spot,
                           const Parameters& vol, const Parameters& r,
                           std::uint64_t num_paths, Rng& rng);

}  // namespace cpp4q::joshi::ch04

#endif  // CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_VANILLA_H_
