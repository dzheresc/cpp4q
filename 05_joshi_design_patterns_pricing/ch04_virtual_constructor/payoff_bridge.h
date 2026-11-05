// Chapter 4: the bridge.
//
// PayOffBridge gives a polymorphic PayOff value semantics: copying the
// bridge clones the pay-off, so classes that hold one (VanillaOption, the
// tree products, the path-dependent products) can use the compiler's
// copy operations. The inner pointer is never null.

#ifndef CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PAYOFF_BRIDGE_H_
#define CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PAYOFF_BRIDGE_H_

#include <memory>

#include "ch04_virtual_constructor/payoff3.h"

namespace cpp4q::joshi::ch04 {

class PayOffBridge {
 public:
  // Implicit, so a PayOffCall can be passed where a bridge is expected.
  PayOffBridge(const PayOff& inner);  // NOLINT(google-explicit-constructor)
  explicit PayOffBridge(std::unique_ptr<PayOff> inner);

  PayOffBridge(const PayOffBridge& other);
  PayOffBridge& operator=(const PayOffBridge& other);
  PayOffBridge(PayOffBridge&& other) noexcept = default;
  PayOffBridge& operator=(PayOffBridge&& other) noexcept = default;
  ~PayOffBridge() = default;

  double operator()(double spot) const { return (*inner_)(spot); }
  const PayOff& get() const { return *inner_; }

 private:
  std::unique_ptr<PayOff> inner_;
};

}  // namespace cpp4q::joshi::ch04

#endif  // CPP4Q_JOSHI_CH04_VIRTUAL_CONSTRUCTOR_PAYOFF_BRIDGE_H_
