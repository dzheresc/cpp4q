#include "ch10_interfaces/interfaces.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch10 {
namespace {

double D2(double strike, double tau, const BlackScholesModel& m) {
  const double sd = m.volatility * std::sqrt(tau);
  return (std::log(m.stock_price / strike) +
          (m.risk_free_rate - 0.5 * m.volatility * m.volatility) * tau) /
         sd;
}

// The integrand of PriceByIntegration as a RealFunction of y = log S(T).
class PayoffTimesDensity final : public RealFunction {
 public:
  PayoffTimesDensity(const PathIndependentOption& option, double mean,
                     double sd)
      : option_(option), mean_(mean), sd_(sd) {}
  double Evaluate(double y) const override {
    return option_.PayoffAtMaturity(std::exp(y)) *
           ch03::NormPdf((y - mean_) / sd_) / sd_;
  }

 private:
  const PathIndependentOption& option_;
  double mean_;
  double sd_;
};

}  // namespace

StrikeAndMaturity::StrikeAndMaturity(double strike, double maturity)
    : strike_(strike), maturity_(maturity) {
  if (!(strike > 0)) throw std::invalid_argument("strike must be positive");
  if (!(maturity >= 0)) throw std::invalid_argument("maturity must be >= 0");
}

double StrikeAndMaturity::TimeTo(const BlackScholesModel& m) const {
  const double tau = maturity_ - m.date;
  if (!(tau > 0)) throw std::invalid_argument("option has expired");
  return tau;
}

double EuropeanCall::PayoffAtMaturity(double s) const {
  return std::max(s - strike_, 0.0);
}
double EuropeanCall::Price(const BlackScholesModel& m) const {
  return ch03::BlackScholesCallPrice(strike_, TimeTo(m), m.stock_price,
                                     m.volatility, m.risk_free_rate);
}
double EuropeanCall::Delta(const BlackScholesModel& m) const {
  return ch03::BlackScholesCallDelta(strike_, TimeTo(m), m.stock_price,
                                     m.volatility, m.risk_free_rate);
}

double EuropeanPut::PayoffAtMaturity(double s) const {
  return std::max(strike_ - s, 0.0);
}
double EuropeanPut::Price(const BlackScholesModel& m) const {
  return ch03::BlackScholesPutPrice(strike_, TimeTo(m), m.stock_price,
                                    m.volatility, m.risk_free_rate);
}

double DigitalCall::PayoffAtMaturity(double s) const {
  return s > strike_ ? 1.0 : 0.0;
}
double DigitalCall::Price(const BlackScholesModel& m) const {
  const double tau = TimeTo(m);
  return std::exp(-m.risk_free_rate * tau) *
         ch03::NormCdf(D2(strike_, tau, m));
}

double DigitalPut::PayoffAtMaturity(double s) const {
  return s < strike_ ? 1.0 : 0.0;
}
double DigitalPut::Price(const BlackScholesModel& m) const {
  const double tau = TimeTo(m);
  return std::exp(-m.risk_free_rate * tau) *
         ch03::NormCdf(-D2(strike_, tau, m));
}

Estimate MonteCarloPricer::Price(const ContinuousTimeOption& option,
                                 const BlackScholesModel& model,
                                 Rng& rng) const {
  if (n_scenarios_ < 2) throw std::invalid_argument("need >= 2 scenarios");
  if (n_steps_ < 1) throw std::invalid_argument("need >= 1 step");
  const int steps = option.IsPathDependent() ? n_steps_ : 1;
  double sum = 0.0;
  double sum_sq = 0.0;
  for (int i = 0; i < n_scenarios_; ++i) {
    const std::vector<double> path = ch09::GeneratePricePath(
        model, option.maturity(), steps, ch09::Measure::kRiskNeutral, rng);
    const double payoff = option.Payoff(path);
    sum += payoff;
    sum_sq += payoff * payoff;
  }
  const double n = n_scenarios_;
  const double mean = sum / n;
  const double var = (sum_sq - n * mean * mean) / (n - 1);
  const double df =
      std::exp(-model.risk_free_rate * (option.maturity() - model.date));
  return {df * mean, df * std::sqrt(std::max(var, 0.0) / n)};
}

double Integrate(const RealFunction& f, double a, double b, int n_points) {
  if (n_points < 1) throw std::invalid_argument("n_points must be >= 1");
  const double h = (b - a) / n_points;
  double sum = 0.0;
  for (int i = 0; i < n_points; ++i) sum += f.Evaluate(a + (i + 0.5) * h);
  return sum * h;
}

double PriceByIntegration(const PathIndependentOption& option,
                          const BlackScholesModel& model, int n_points) {
  const double tau = option.maturity() - model.date;
  if (!(tau > 0)) throw std::invalid_argument("option has expired");
  const double sd = model.volatility * std::sqrt(tau);
  const double mean =
      std::log(model.stock_price) +
      (model.risk_free_rate - 0.5 * model.volatility * model.volatility) * tau;
  const PayoffTimesDensity integrand(option, mean, sd);
  return std::exp(-model.risk_free_rate * tau) *
         Integrate(integrand, mean - 10 * sd, mean + 10 * sd, n_points);
}

}  // namespace cpp4q::armstrong::ch10
