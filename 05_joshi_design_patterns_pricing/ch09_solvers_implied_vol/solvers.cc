#include "ch09_solvers_implied_vol/solvers.h"

#include <algorithm>
#include <cmath>

#include "appendix_a_black_scholes/black_scholes.h"

namespace cpp4q::joshi::ch09 {

double BsCall::Price(double vol) const {
  return BlackScholesCall(spot_, strike_, r_, d_, vol, expiry_);
}

double BsCall::Vega(double vol) const {
  return BlackScholesCallVega(spot_, strike_, r_, d_, vol, expiry_);
}

double BsCall::InflectionVol() const {
  const double log_moneyness = std::log(spot_ / strike_) + (r_ - d_) * expiry_;
  return std::sqrt(std::max(2.0 * std::fabs(log_moneyness) / expiry_, 1e-4));
}

namespace {

void CheckBounds(const BsCall& call, double price) {
  const double lower = call.Price(0.0);
  const double upper = call.Price(1e6);
  if (!(price > lower && price < upper)) {
    throw std::domain_error("ImpliedVol: price outside no-arbitrage bounds");
  }
}

}  // namespace

double ImpliedVolBisection(const BsCall& call, double price,
                           double tolerance) {
  CheckBounds(call, price);
  double high = 1.0;
  while (call.Price(high) < price) high *= 2.0;
  return Bisection(price, 0.0, high, tolerance,
                   [&call](double v) { return call.Price(v); });
}

double ImpliedVolNewton(const BsCall& call, double price,
                        double tolerance) {
  CheckBounds(call, price);
  return NewtonRaphson<BsCall, &BsCall::Price, &BsCall::Vega>(
      price, call.InflectionVol(), tolerance, call);
}

}  // namespace cpp4q::joshi::ch09
