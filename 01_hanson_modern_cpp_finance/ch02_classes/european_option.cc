#include "ch02_classes/european_option.h"

#include <algorithm>
#include <format>
#include <stdexcept>

namespace cpp4q::hanson::ch02 {

EuropeanOption::EuropeanOption(OptionType type, double strike,
                               double time_to_expiry)
    : type_(type), strike_(strike), time_to_expiry_(time_to_expiry) {
  if (!(strike > 0.0)) {
    throw std::invalid_argument("EuropeanOption: strike must be positive");
  }
  if (!(time_to_expiry >= 0.0)) {
    throw std::invalid_argument("EuropeanOption: expiry must be >= 0");
  }
}

ch01::BlackScholesInputs EuropeanOption::ToInputs(
    const MarketData& market) const {
  return {.spot = market.spot,
          .strike = strike_,
          .rate = market.rate,
          .dividend_yield = market.dividend_yield,
          .volatility = market.volatility,
          .time_to_expiry = time_to_expiry_};
}

double EuropeanOption::Price(const MarketData& market) const {
  return ch01::BlackScholesPrice(type_, ToInputs(market));
}

ch01::Greeks EuropeanOption::Greeks(const MarketData& market) const {
  return ch01::BlackScholesGreeks(type_, ToInputs(market));
}

double EuropeanOption::Payoff(double spot_at_expiry) const {
  return ch01::Payoff(type_, spot_at_expiry, strike_);
}

void EuropeanOption::Advance(double dt) {
  time_to_expiry_ = std::max(0.0, time_to_expiry_ - dt);
}

std::ostream& operator<<(std::ostream& os, const EuropeanOption& o) {
  return os << std::format("{}(K={:.2f}, T={:.4f})",
                           o.type_ == OptionType::kCall ? "Call" : "Put",
                           o.strike_, o.time_to_expiry_);
}

}  // namespace cpp4q::hanson::ch02
