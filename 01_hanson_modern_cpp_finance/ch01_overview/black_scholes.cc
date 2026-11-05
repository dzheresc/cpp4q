#include "ch01_overview/black_scholes.h"

#include <algorithm>
#include <cmath>

namespace cpp4q::hanson::ch01 {
namespace {

struct D1D2 {
  double d1;
  double d2;
};

D1D2 ComputeD1D2(const BlackScholesInputs& in) {
  const double vol_sqrt_t = in.volatility * std::sqrt(in.time_to_expiry);
  const double d1 =
      (std::log(in.spot / in.strike) +
       (in.rate - in.dividend_yield + 0.5 * in.volatility * in.volatility) *
           in.time_to_expiry) /
      vol_sqrt_t;
  return {d1, d1 - vol_sqrt_t};
}

bool IsDegenerate(const BlackScholesInputs& in) {
  return in.volatility <= 0.0 || in.time_to_expiry <= 0.0;
}

}  // namespace

double NormPdf(double x) { return kInvSqrt2Pi * std::exp(-0.5 * x * x); }

double NormCdf(double x) { return 0.5 * std::erfc(-x / std::numbers::sqrt2); }

double DiscountFactor(double rate, double time) {
  return std::exp(-rate * time);
}

double BlackScholesPrice(OptionType type, const BlackScholesInputs& in) {
  const double df_r = DiscountFactor(in.rate, in.time_to_expiry);
  const double df_q = DiscountFactor(in.dividend_yield, in.time_to_expiry);
  if (IsDegenerate(in)) {
    const double forward = in.spot * df_q / df_r;
    return df_r * Payoff(type, forward, in.strike);
  }
  const auto [d1, d2] = ComputeD1D2(in);
  if (type == OptionType::kCall) {
    return in.spot * df_q * NormCdf(d1) - in.strike * df_r * NormCdf(d2);
  }
  return in.strike * df_r * NormCdf(-d2) - in.spot * df_q * NormCdf(-d1);
}

Greeks BlackScholesGreeks(OptionType type, const BlackScholesInputs& in) {
  if (IsDegenerate(in)) return {};
  const auto [d1, d2] = ComputeD1D2(in);
  const double t = in.time_to_expiry;
  const double sqrt_t = std::sqrt(t);
  const double df_r = DiscountFactor(in.rate, t);
  const double df_q = DiscountFactor(in.dividend_yield, t);
  const double pdf_d1 = NormPdf(d1);

  Greeks g;
  g.gamma = df_q * pdf_d1 / (in.spot * in.volatility * sqrt_t);
  g.vega = in.spot * df_q * pdf_d1 * sqrt_t;
  const double decay = -in.spot * df_q * pdf_d1 * in.volatility / (2 * sqrt_t);
  if (type == OptionType::kCall) {
    g.delta = df_q * NormCdf(d1);
    g.theta = decay - in.rate * in.strike * df_r * NormCdf(d2) +
              in.dividend_yield * in.spot * df_q * NormCdf(d1);
    g.rho = in.strike * t * df_r * NormCdf(d2);
  } else {
    g.delta = -df_q * NormCdf(-d1);
    g.theta = decay + in.rate * in.strike * df_r * NormCdf(-d2) -
              in.dividend_yield * in.spot * df_q * NormCdf(-d1);
    g.rho = -in.strike * t * df_r * NormCdf(-d2);
  }
  return g;
}

}  // namespace cpp4q::hanson::ch01
