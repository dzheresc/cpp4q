#include "ch09_boost/boost_pricing.h"

#include <algorithm>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/quadrature/gauss_kronrod.hpp>
#include <boost/math/tools/roots.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <tuple>

namespace cpp4q::hanson::ch09 {
namespace {

constexpr double kMinVol = 1e-6;
constexpr double kMaxVol = 5.0;

const boost::math::normal& StandardNormal() {
  static const boost::math::normal kNormal(0.0, 1.0);
  return kNormal;
}

bool WithinArbitrageBounds(OptionType type, const BlackScholesInputs& in,
                           double price) {
  const double df_r = std::exp(-in.rate * in.time_to_expiry);
  const double df_q = std::exp(-in.dividend_yield * in.time_to_expiry);
  const double fwd_s = in.spot * df_q;
  const double pv_k = in.strike * df_r;
  const double lower = type == OptionType::kCall ? std::max(fwd_s - pv_k, 0.0)
                                                 : std::max(pv_k - fwd_s, 0.0);
  const double upper = type == OptionType::kCall ? fwd_s : pv_k;
  return price > lower && price < upper;
}

BlackScholesInputs WithVol(BlackScholesInputs in, double vol) {
  in.volatility = vol;
  return in;
}

}  // namespace

double BlackScholesBoost(OptionType type, const BlackScholesInputs& in) {
  using boost::math::cdf;
  const auto& n = StandardNormal();
  const double t = in.time_to_expiry;
  const double vst = in.volatility * std::sqrt(t);
  const double d1 =
      (std::log(in.spot / in.strike) + (in.rate - in.dividend_yield) * t) /
          vst +
      0.5 * vst;
  const double d2 = d1 - vst;
  const double df_r = std::exp(-in.rate * t);
  const double df_q = std::exp(-in.dividend_yield * t);
  if (type == OptionType::kCall) {
    return in.spot * df_q * cdf(n, d1) - in.strike * df_r * cdf(n, d2);
  }
  return in.strike * df_r * cdf(n, -d2) - in.spot * df_q * cdf(n, -d1);
}

std::optional<ImpliedVolResult> ImpliedVolToms748(OptionType type,
                                                  const BlackScholesInputs& in,
                                                  double price) {
  if (!WithinArbitrageBounds(type, in, price)) return std::nullopt;
  const auto f = [&](double vol) {
    return ch01::BlackScholesPrice(type, WithVol(in, vol)) - price;
  };
  std::uintmax_t max_iter = 100;
  const auto [lo, hi] = boost::math::tools::toms748_solve(
      f, kMinVol, kMaxVol, boost::math::tools::eps_tolerance<double>(50),
      max_iter);
  return ImpliedVolResult{0.5 * (lo + hi), max_iter};
}

std::optional<ImpliedVolResult> ImpliedVolNewton(OptionType type,
                                                 const BlackScholesInputs& in,
                                                 double price) {
  if (!WithinArbitrageBounds(type, in, price)) return std::nullopt;
  const auto f = [&](double vol) {
    const auto with_vol = WithVol(in, vol);
    return std::make_tuple(ch01::BlackScholesPrice(type, with_vol) - price,
                           ch01::BlackScholesGreeks(type, with_vol).vega);
  };
  const double guess = std::clamp(
      std::sqrt(2.0 * std::numbers::pi / in.time_to_expiry) * price / in.spot,
      0.05, 2.0);
  std::uintmax_t max_iter = 100;
  const double vol = boost::math::tools::newton_raphson_iterate(
      f, guess, kMinVol, kMaxVol, std::numeric_limits<double>::digits - 4,
      max_iter);
  return ImpliedVolResult{vol, max_iter};
}

double PriceByQuadrature(OptionType type, const BlackScholesInputs& in) {
  const double t = in.time_to_expiry;
  const double vst = in.volatility * std::sqrt(t);
  const double mu =
      std::log(in.spot) +
      (in.rate - in.dividend_yield - 0.5 * in.volatility * in.volatility) * t;
  // Integrate over x = log S_T in mu +/- 10 sd; the tails are negligible.
  const auto integrand = [&](double x) {
    const double z = (x - mu) / vst;
    return ch01::Payoff(type, std::exp(x), in.strike) * ch01::NormPdf(z) / vst;
  };
  // Split at log K so the payoff kink sits on an interval boundary.
  const double log_k = std::log(in.strike);
  const double lo = mu - 10.0 * vst;
  const double hi = mu + 10.0 * vst;
  using Gk = boost::math::quadrature::gauss_kronrod<double, 61>;
  double total = 0.0;
  if (log_k > lo)
    total += Gk::integrate(integrand, lo, std::min(log_k, hi), 15, 1e-12);
  if (log_k < hi)
    total += Gk::integrate(integrand, std::max(log_k, lo), hi, 15, 1e-12);
  return std::exp(-in.rate * t) * total;
}

double NormalQuantile(double p) {
  return boost::math::quantile(StandardNormal(), p);
}

}  // namespace cpp4q::hanson::ch09
