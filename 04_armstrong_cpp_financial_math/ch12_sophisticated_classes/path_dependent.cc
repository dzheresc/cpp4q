#include "ch12_sophisticated_classes/path_dependent.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdexcept>

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch12 {
namespace {

using ch03::NormCdf;

double TimeTo(double maturity, const BlackScholesModel& m) {
  const double tau = maturity - m.date;
  if (!(tau > 0)) throw std::invalid_argument("option has expired");
  return tau;
}

}  // namespace

PathDependentOption::PathDependentOption(double maturity)
    : maturity_(maturity) {
  if (!(maturity >= 0)) throw std::invalid_argument("maturity must be >= 0");
}

BarrierOption::BarrierOption(double strike, double barrier, double maturity)
    : PathDependentOption(maturity), strike_(strike), barrier_(barrier) {
  if (!(strike > 0 && barrier > 0)) {
    throw std::invalid_argument("strike and barrier must be positive");
  }
}

double BarrierOption::Payoff(std::span<const double> prices) const {
  return Breached(prices) == IsKnockIn() ? VanillaPayoff(prices.back()) : 0.0;
}

bool UpAndOutCall::Breached(std::span<const double> p) const {
  return std::ranges::any_of(p, [this](double s) { return s >= barrier_; });
}
double UpAndOutCall::VanillaPayoff(double s) const {
  return std::max(s - strike_, 0.0);
}
bool UpAndInCall::Breached(std::span<const double> p) const {
  return std::ranges::any_of(p, [this](double s) { return s >= barrier_; });
}
double UpAndInCall::VanillaPayoff(double s) const {
  return std::max(s - strike_, 0.0);
}
bool DownAndOutPut::Breached(std::span<const double> p) const {
  return std::ranges::any_of(p, [this](double s) { return s <= barrier_; });
}
double DownAndOutPut::VanillaPayoff(double s) const {
  return std::max(strike_ - s, 0.0);
}
bool DownAndInPut::Breached(std::span<const double> p) const {
  return std::ranges::any_of(p, [this](double s) { return s <= barrier_; });
}
double DownAndInPut::VanillaPayoff(double s) const {
  return std::max(strike_ - s, 0.0);
}

AsianOption::AsianOption(OptionType type, double strike, double maturity)
    : PathDependentOption(maturity), type_(type), strike_(strike) {
  if (!(strike > 0)) throw std::invalid_argument("strike must be positive");
}

double AsianOption::Payoff(std::span<const double> prices) const {
  const double a = Average(prices);
  return type_ == OptionType::kCall ? std::max(a - strike_, 0.0)
                                    : std::max(strike_ - a, 0.0);
}

double ArithmeticAsianOption::Average(std::span<const double> p) const {
  double sum = 0.0;
  for (double s : p) sum += s;
  return sum / static_cast<double>(p.size());
}

double GeometricAsianOption::Average(std::span<const double> p) const {
  double sum_log = 0.0;
  for (double s : p) sum_log += std::log(s);
  return std::exp(sum_log / static_cast<double>(p.size()));
}

double GeometricAsianCallPrice(const BlackScholesModel& m, double strike,
                               double maturity, int fixings) {
  if (fixings < 1) throw std::invalid_argument("fixings must be >= 1");
  const double tau = TimeTo(maturity, m);
  const double n = fixings;
  const double sigma = m.volatility;
  // log G = log S + (r - sigma^2/2) mean(t_i) + sigma mean(W(t_i)).
  const double mean = std::log(m.stock_price) +
                      (m.risk_free_rate - 0.5 * sigma * sigma) * tau *
                          (n + 1) / (2 * n);
  const double var =
      sigma * sigma * tau * (n + 1) * (2 * n + 1) / (6 * n * n);
  const double sd = std::sqrt(var);
  const double d1 = (mean - std::log(strike) + var) / sd;
  return std::exp(-m.risk_free_rate * tau) *
         (std::exp(mean + 0.5 * var) * NormCdf(d1) -
          strike * NormCdf(d1 - sd));
}

double UpAndInCallPrice(const BlackScholesModel& m, double strike,
                        double barrier, double maturity) {
  const double tau = TimeTo(maturity, m);
  const double s = m.stock_price;
  const double k = strike;
  const double h = barrier;
  const double r = m.risk_free_rate;
  const double sigma = m.volatility;
  const double call = ch03::BlackScholesCallPrice(k, tau, s, sigma, r);
  if (s >= h || h <= k) return call;  // knocked in already / always in
  const double sd = sigma * std::sqrt(tau);
  const double lambda = (r + 0.5 * sigma * sigma) / (sigma * sigma);
  const double x1 = std::log(s / h) / sd + lambda * sd;
  const double y = std::log(h * h / (s * k)) / sd + lambda * sd;
  const double y1 = std::log(h / s) / sd + lambda * sd;
  const double df = std::exp(-r * tau);
  const double hs = h / s;
  return s * NormCdf(x1) - k * df * NormCdf(x1 - sd) -
         s * std::pow(hs, 2 * lambda) * (NormCdf(-y) - NormCdf(-y1)) +
         k * df * std::pow(hs, 2 * lambda - 2) *
             (NormCdf(-y + sd) - NormCdf(-y1 + sd));
}

double UpAndOutCallPrice(const BlackScholesModel& m, double strike,
                         double barrier, double maturity) {
  const double tau = TimeTo(maturity, m);
  return ch03::BlackScholesCallPrice(strike, tau, m.stock_price, m.volatility,
                                     m.risk_free_rate) -
         UpAndInCallPrice(m, strike, barrier, maturity);
}

double DiscreteBarrierShift(double barrier, double vol, double dt, bool up) {
  constexpr double kBeta = 0.5825971579390106;  // -zeta(1/2) / sqrt(2 pi)
  return barrier * std::exp((up ? kBeta : -kBeta) * vol * std::sqrt(dt));
}

}  // namespace cpp4q::armstrong::ch12

namespace cpp4q::armstrong::ch09 {

std::ostream& operator<<(std::ostream& out, const Estimate& e) {
  return out << e.price << " +- " << e.std_error;
}

}  // namespace cpp4q::armstrong::ch09
