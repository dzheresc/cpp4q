#include "ch03_polymorphism/option.h"

#include <stdexcept>
#include <utility>

namespace cpp4q::hanson::ch03 {

Option::Option(std::unique_ptr<Payoff> payoff, double expiry,
               std::shared_ptr<const YieldCurve> curve)
    : payoff_(std::move(payoff)), expiry_(expiry), curve_(std::move(curve)) {
  if (!payoff_ || !curve_) {
    throw std::invalid_argument("Option: null payoff or curve");
  }
  if (!(expiry_ >= 0.0)) throw std::invalid_argument("Option: expiry < 0");
}

Option::Option(const Option& other)
    : payoff_(other.payoff_->Clone()),
      expiry_(other.expiry_),
      curve_(other.curve_) {}

Option& Option::operator=(const Option& other) {
  if (this != &other) {
    payoff_ = other.payoff_->Clone();
    expiry_ = other.expiry_;
    curve_ = other.curve_;
  }
  return *this;
}

double Option::DiscountedPayoff(double spot) const {
  return curve_->DiscountFactor(expiry_) * (*payoff_)(spot);
}

}  // namespace cpp4q::hanson::ch03
