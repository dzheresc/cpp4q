#include "ch04_virtual_constructor/payoff3.h"

#include <algorithm>

namespace cpp4q::joshi::ch04 {

double PayOffCall::operator()(double spot) const {
  return std::max(spot - strike_, 0.0);
}

double PayOffPut::operator()(double spot) const {
  return std::max(strike_ - spot, 0.0);
}

double PayOffDigitalCall::operator()(double spot) const {
  return spot > strike_ ? 1.0 : 0.0;
}

double PayOffDoubleDigital::operator()(double spot) const {
  return spot >= lower_ && spot <= upper_ ? 1.0 : 0.0;
}

}  // namespace cpp4q::joshi::ch04
