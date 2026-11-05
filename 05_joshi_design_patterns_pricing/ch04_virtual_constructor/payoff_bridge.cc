#include "ch04_virtual_constructor/payoff_bridge.h"

#include <stdexcept>
#include <utility>

namespace cpp4q::joshi::ch04 {

PayOffBridge::PayOffBridge(const PayOff& inner) : inner_(inner.Clone()) {}

PayOffBridge::PayOffBridge(std::unique_ptr<PayOff> inner)
    : inner_(std::move(inner)) {
  if (!inner_) throw std::invalid_argument("PayOffBridge: null pay-off");
}

PayOffBridge::PayOffBridge(const PayOffBridge& other)
    : inner_(other.inner_->Clone()) {}

PayOffBridge& PayOffBridge::operator=(const PayOffBridge& other) {
  // Clone first: if it throws, *this is unchanged (strong guarantee), and
  // self-assignment needs no special case.
  inner_ = other.inner_->Clone();
  return *this;
}

}  // namespace cpp4q::joshi::ch04
