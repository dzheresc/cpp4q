#include "ch03_polymorphism/payoff.h"

#include <algorithm>
#include <stdexcept>

namespace cpp4q::hanson::ch03 {

double CallPayoff::operator()(double spot) const {
  return std::max(spot - strike_, 0.0);
}
std::unique_ptr<Payoff> CallPayoff::Clone() const {
  return std::make_unique<CallPayoff>(*this);
}

double PutPayoff::operator()(double spot) const {
  return std::max(strike_ - spot, 0.0);
}
std::unique_ptr<Payoff> PutPayoff::Clone() const {
  return std::make_unique<PutPayoff>(*this);
}

double DigitalCallPayoff::operator()(double spot) const {
  return spot > strike_ ? cash_ : 0.0;
}
std::unique_ptr<Payoff> DigitalCallPayoff::Clone() const {
  return std::make_unique<DigitalCallPayoff>(*this);
}

DoubleDigitalPayoff::DoubleDigitalPayoff(double lower, double upper)
    : lower_(lower), upper_(upper) {
  if (!(lower < upper)) {
    throw std::invalid_argument("DoubleDigitalPayoff: need lower < upper");
  }
}
double DoubleDigitalPayoff::operator()(double spot) const {
  return (spot >= lower_ && spot <= upper_) ? 1.0 : 0.0;
}
std::unique_ptr<Payoff> DoubleDigitalPayoff::Clone() const {
  return std::make_unique<DoubleDigitalPayoff>(*this);
}

}  // namespace cpp4q::hanson::ch03
