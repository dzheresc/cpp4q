#include "ch08_user_defined_types/options.h"

#include <algorithm>
#include <stdexcept>

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch08 {
namespace {

void CheckContract(double strike, double maturity) {
  if (!(strike > 0)) throw std::invalid_argument("strike must be positive");
  if (!(maturity >= 0)) throw std::invalid_argument("maturity must be >= 0");
}

double TimeToMaturity(double maturity, const BlackScholesModel& m) {
  const double tau = maturity - m.date;
  if (!(tau > 0)) throw std::invalid_argument("option has expired");
  return tau;
}

}  // namespace

CallOption::CallOption(double strike, double maturity)
    : strike_(strike), maturity_(maturity) {
  CheckContract(strike, maturity);
}

double CallOption::Payoff(double s) const { return std::max(s - strike_, 0.0); }

double CallOption::Price(const BlackScholesModel& m) const {
  return ch03::BlackScholesCallPrice(strike_, TimeToMaturity(maturity_, m),
                                     m.stock_price, m.volatility,
                                     m.risk_free_rate);
}

PutOption::PutOption(double strike, double maturity)
    : strike_(strike), maturity_(maturity) {
  CheckContract(strike, maturity);
}

double PutOption::Payoff(double s) const { return std::max(strike_ - s, 0.0); }

double PutOption::Price(const BlackScholesModel& m) const {
  return ch03::BlackScholesPutPrice(strike_, TimeToMaturity(maturity_, m),
                                    m.stock_price, m.volatility,
                                    m.risk_free_rate);
}

}  // namespace cpp4q::armstrong::ch08
