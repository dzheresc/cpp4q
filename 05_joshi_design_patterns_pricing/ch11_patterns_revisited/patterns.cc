#include "ch11_patterns_revisited/patterns.h"

#include <algorithm>
#include <stdexcept>

namespace cpp4q::joshi::ch11 {

PayOffPortfolio& PayOffPortfolio::Add(double weight,
                                      ch04::PayOffBridge payoff) {
  legs_.emplace_back(weight, std::move(payoff));
  return *this;
}

double PayOffPortfolio::operator()(double spot) const {
  double value = 0.0;
  for (const auto& [weight, payoff] : legs_) value += weight * payoff(spot);
  return value;
}

PayOffCollar::PayOffCollar(ch04::PayOffBridge inner, double floor, double cap)
    : inner_(std::move(inner)), floor_(floor), cap_(cap) {
  if (!(floor_ <= cap_)) throw std::invalid_argument("PayOffCollar: floor > cap");
}

double PayOffCollar::operator()(double spot) const {
  return std::clamp(inner_(spot), floor_, cap_);
}

PayOffPortfolio MakeButterfly(double center, double wing) {
  PayOffPortfolio fly;
  fly.Add(1.0, ch04::PayOffCall(center - wing))
      .Add(-2.0, ch04::PayOffCall(center))
      .Add(1.0, ch04::PayOffCall(center + wing));
  return fly;
}

}  // namespace cpp4q::joshi::ch11
