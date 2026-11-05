#include "ch07_sensitivities/greeks.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch07 {
namespace {

double DiscountedCall(const BlackScholesInputs& in, double spot, double z) {
  const double t = in.maturity;
  const double st =
      spot * std::exp((in.rate - in.dividend - 0.5 * in.vol * in.vol) * t +
                      in.vol * std::sqrt(t) * z);
  return std::exp(-in.rate * t) * std::max(st - in.strike, 0.0);
}

}  // namespace

Estimate FiniteDifferenceDelta(const BlackScholesInputs& in, double rel_bump,
                               FiniteDifference kind,
                               bool common_random_numbers, std::size_t paths,
                               std::uint64_t seed) {
  const double h = rel_bump * in.spot;
  const bool central = kind == FiniteDifference::kCentral;
  const double up = in.spot + h;
  const double down = central ? in.spot - h : in.spot;
  const double width = central ? 2.0 * h : h;
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats st;
  for (std::size_t i = 0; i < paths; ++i) {
    const double z1 = s.Normal();
    const double z2 = common_random_numbers ? z1 : s.Normal();
    st.Add((DiscountedCall(in, up, z1) - DiscountedCall(in, down, z2)) / width);
  }
  return st.ToEstimate();
}

CallGreeks EuropeanCallGreeks(const BlackScholesInputs& in, std::size_t paths,
                              std::uint64_t seed) {
  const double t = in.maturity;
  const double sqrt_t = std::sqrt(t);
  const double sigma = in.vol;
  const double drift = (in.rate - in.dividend - 0.5 * sigma * sigma) * t;
  const double df = std::exp(-in.rate * t);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats price, pw_delta, pw_vega, lr_delta, lr_vega, lr_gamma;
  for (std::size_t i = 0; i < paths; ++i) {
    const double z = s.Normal();
    const double st = in.spot * std::exp(drift + sigma * sqrt_t * z);
    const double payoff = df * std::max(st - in.strike, 0.0);
    const double itm = st > in.strike ? df : 0.0;
    price.Add(payoff);
    // dS(T)/dS0 = S(T)/S0, dS(T)/dsigma = S(T) (-sigma T + sqrt(T) Z).
    pw_delta.Add(itm * st / in.spot);
    pw_vega.Add(itm * st * (-sigma * t + sqrt_t * z));
    // Scores of the lognormal density of S(T) (Glasserman 7.3.1).
    const double sd = sigma * sqrt_t;
    lr_delta.Add(payoff * z / (in.spot * sd));
    lr_vega.Add(payoff * ((z * z - 1.0) / sigma - z * sqrt_t));
    lr_gamma.Add(payoff * (z * z - 1.0 - z * sd) /
                 (in.spot * in.spot * sd * sd));
  }
  return {price.ToEstimate(),    pw_delta.ToEstimate(), pw_vega.ToEstimate(),
          lr_delta.ToEstimate(), lr_vega.ToEstimate(),  lr_gamma.ToEstimate()};
}

DigitalGreeks DigitalCallGreeks(const BlackScholesInputs& in, std::size_t paths,
                                std::uint64_t seed) {
  const double t = in.maturity;
  const double sd = in.vol * std::sqrt(t);
  const double drift = (in.rate - in.dividend - 0.5 * in.vol * in.vol) * t;
  const double df = std::exp(-in.rate * t);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats price, pw, lr;
  for (std::size_t i = 0; i < paths; ++i) {
    const double z = s.Normal();
    const double payoff =
        in.spot * std::exp(drift + sd * z) > in.strike ? df : 0.0;
    price.Add(payoff);
    pw.Add(0.0);
    lr.Add(payoff * z / (in.spot * sd));
  }
  return {price.ToEstimate(), pw.ToEstimate(), lr.ToEstimate()};
}

AsianDelta AsianCallDelta(const BlackScholesInputs& in, int fixings,
                          std::size_t paths, std::uint64_t seed) {
  if (fixings < 1) throw std::invalid_argument("need at least one fixing");
  const double dt = in.maturity / fixings;
  const double sd = in.vol * std::sqrt(dt);
  const double drift = (in.rate - in.dividend - 0.5 * in.vol * in.vol) * dt;
  const double df = std::exp(-in.rate * in.maturity);
  ch02::Sampler s{ch02::Mrg32k3a(seed)};
  ch01::RunningStats price, pw, lr;
  for (std::size_t i = 0; i < paths; ++i) {
    double log_s = std::log(in.spot);
    double sum = 0.0;
    double z1 = 0.0;
    for (int k = 0; k < fixings; ++k) {
      const double z = s.Normal();
      if (k == 0) z1 = z;
      log_s += drift + sd * z;
      sum += std::exp(log_s);
    }
    const double avg = sum / fixings;
    const double payoff = df * std::max(avg - in.strike, 0.0);
    price.Add(payoff);
    pw.Add(avg > in.strike ? df * avg / in.spot : 0.0);
    // S0 enters the path density only through S(t_1).
    lr.Add(payoff * z1 / (in.spot * sd));
  }
  return {price.ToEstimate(), pw.ToEstimate(), lr.ToEstimate()};
}

}  // namespace cpp4q::glasserman::ch07
