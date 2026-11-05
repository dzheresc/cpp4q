#include "appendix_a_black_scholes/black_scholes.h"

#include <algorithm>
#include <cmath>

#include "appendix_b_distributions/normals.h"

namespace cpp4q::joshi {
namespace {

struct D12 {
  double d1;
  double d2;
  double sd;  // vol * sqrt(expiry)
};

D12 ComputeD12(double spot, double strike, double r, double d, double vol,
               double expiry) {
  const double sd = vol * std::sqrt(expiry);
  const double d1 =
      (std::log(spot / strike) + (r - d) * expiry) / sd + 0.5 * sd;
  return {d1, d1 - sd, sd};
}

bool Degenerate(double vol, double expiry) {
  return vol <= 0.0 || expiry <= 0.0;
}

}  // namespace

double BlackScholesCall(double spot, double strike, double r, double d,
                        double vol, double expiry) {
  const double fwd_spot = spot * std::exp(-d * expiry);
  const double df_strike = strike * std::exp(-r * expiry);
  if (Degenerate(vol, expiry)) return std::max(fwd_spot - df_strike, 0.0);
  const D12 k = ComputeD12(spot, strike, r, d, vol, expiry);
  return fwd_spot * CumulativeNormal(k.d1) -
         df_strike * CumulativeNormal(k.d2);
}

double BlackScholesPut(double spot, double strike, double r, double d,
                       double vol, double expiry) {
  const double fwd_spot = spot * std::exp(-d * expiry);
  const double df_strike = strike * std::exp(-r * expiry);
  if (Degenerate(vol, expiry)) return std::max(df_strike - fwd_spot, 0.0);
  const D12 k = ComputeD12(spot, strike, r, d, vol, expiry);
  return df_strike * CumulativeNormal(-k.d2) -
         fwd_spot * CumulativeNormal(-k.d1);
}

double BlackScholesDigitalCall(double spot, double strike, double r, double d,
                               double vol, double expiry) {
  const double df = std::exp(-r * expiry);
  if (Degenerate(vol, expiry)) {
    return spot * std::exp((r - d) * std::max(expiry, 0.0)) > strike ? df
                                                                     : 0.0;
  }
  return df * CumulativeNormal(ComputeD12(spot, strike, r, d, vol, expiry).d2);
}

double BlackScholesDigitalPut(double spot, double strike, double r, double d,
                              double vol, double expiry) {
  const double df = std::exp(-r * expiry);
  if (Degenerate(vol, expiry)) {
    return spot * std::exp((r - d) * std::max(expiry, 0.0)) < strike ? df
                                                                     : 0.0;
  }
  return df *
         CumulativeNormal(-ComputeD12(spot, strike, r, d, vol, expiry).d2);
}

double BlackScholesCallDelta(double spot, double strike, double r, double d,
                             double vol, double expiry) {
  const double q = std::exp(-d * expiry);
  if (Degenerate(vol, expiry)) {
    return spot * q > strike * std::exp(-r * expiry) ? q : 0.0;
  }
  return q * CumulativeNormal(ComputeD12(spot, strike, r, d, vol, expiry).d1);
}

double BlackScholesCallGamma(double spot, double strike, double r, double d,
                             double vol, double expiry) {
  if (Degenerate(vol, expiry)) return 0.0;
  const D12 k = ComputeD12(spot, strike, r, d, vol, expiry);
  return std::exp(-d * expiry) * NormalDensity(k.d1) / (spot * k.sd);
}

double BlackScholesCallVega(double spot, double strike, double r, double d,
                            double vol, double expiry) {
  if (Degenerate(vol, expiry)) return 0.0;
  const D12 k = ComputeD12(spot, strike, r, d, vol, expiry);
  return spot * std::exp(-d * expiry) * std::sqrt(expiry) *
         NormalDensity(k.d1);
}

}  // namespace cpp4q::joshi
