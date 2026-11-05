#include "ch01_foundations/black_scholes.h"

#include <cmath>
#include <stdexcept>

namespace cpp4q::glasserman::ch01 {
namespace {

struct D12 {
  double d1;
  double d2;
};

D12 ComputeD12(const BlackScholesInputs& in) {
  const double sd = in.vol * std::sqrt(in.maturity);
  const double d1 =
      (std::log(in.spot / in.strike) +
       (in.rate - in.dividend + 0.5 * in.vol * in.vol) * in.maturity) /
      sd;
  return {d1, d1 - sd};
}

void Validate(const BlackScholesInputs& in) {
  if (!(in.spot > 0.0 && in.strike > 0.0 && in.vol > 0.0 &&
        in.maturity > 0.0)) {
    throw std::invalid_argument(
        "Black-Scholes needs positive spot, strike, vol and maturity");
  }
}

}  // namespace

double NormalPdf(double x) { return kInvSqrt2Pi * std::exp(-0.5 * x * x); }

double NormalCdf(double x) { return 0.5 * std::erfc(-x / kSqrt2); }

double BlackScholesPrice(OptionType type, const BlackScholesInputs& in) {
  Validate(in);
  const auto [d1, d2] = ComputeD12(in);
  const double df = std::exp(-in.rate * in.maturity);
  const double fwd_df = in.spot * std::exp(-in.dividend * in.maturity);
  if (type == OptionType::kCall) {
    return fwd_df * NormalCdf(d1) - in.strike * df * NormalCdf(d2);
  }
  return in.strike * df * NormalCdf(-d2) - fwd_df * NormalCdf(-d1);
}

double BlackScholesDelta(OptionType type, const BlackScholesInputs& in) {
  Validate(in);
  const double q = std::exp(-in.dividend * in.maturity);
  const double n1 = NormalCdf(ComputeD12(in).d1);
  return type == OptionType::kCall ? q * n1 : q * (n1 - 1.0);
}

double BlackScholesGamma(const BlackScholesInputs& in) {
  Validate(in);
  const double q = std::exp(-in.dividend * in.maturity);
  return q * NormalPdf(ComputeD12(in).d1) /
         (in.spot * in.vol * std::sqrt(in.maturity));
}

double BlackScholesVega(const BlackScholesInputs& in) {
  Validate(in);
  const double q = std::exp(-in.dividend * in.maturity);
  return in.spot * q * NormalPdf(ComputeD12(in).d1) * std::sqrt(in.maturity);
}

double DigitalCallPrice(const BlackScholesInputs& in) {
  Validate(in);
  return std::exp(-in.rate * in.maturity) * NormalCdf(ComputeD12(in).d2);
}

double DigitalCallDelta(const BlackScholesInputs& in) {
  Validate(in);
  return std::exp(-in.rate * in.maturity) * NormalPdf(ComputeD12(in).d2) /
         (in.spot * in.vol * std::sqrt(in.maturity));
}

double GeometricAsianCallPrice(const BlackScholesInputs& in, int fixings) {
  Validate(in);
  if (fixings < 1) throw std::invalid_argument("need at least one fixing");
  const double n = fixings;
  const double t = in.maturity;
  // log G = log S0 + (r - q - vol^2/2) * mean(t_i) + vol * mean(W(t_i)).
  const double mean_time = t * (n + 1.0) / (2.0 * n);
  const double var_time = t * (n + 1.0) * (2.0 * n + 1.0) / (6.0 * n * n);
  const double mu = std::log(in.spot) +
                    (in.rate - in.dividend - 0.5 * in.vol * in.vol) * mean_time;
  const double sigma = in.vol * std::sqrt(var_time);
  const double d2 = (mu - std::log(in.strike)) / sigma;
  const double d1 = d2 + sigma;
  return std::exp(-in.rate * t) *
         (std::exp(mu + 0.5 * sigma * sigma) * NormalCdf(d1) -
          in.strike * NormalCdf(d2));
}

}  // namespace cpp4q::glasserman::ch01
