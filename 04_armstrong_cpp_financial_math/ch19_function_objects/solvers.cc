#include "ch19_function_objects/solvers.h"

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch19 {

double IntegrateStdFunction(const std::function<double(double)>& f, double a,
                            double b, int n_points) {
  if (n_points < 1) throw std::invalid_argument("n_points must be >= 1");
  const double h = (b - a) / n_points;
  double sum = 0.0;
  for (int i = 0; i < n_points; ++i) sum += f(a + (i + 0.5) * h);
  return sum * h;
}

RootResult Bisect(const std::function<double(double)>& f, double lo,
                  double hi, double tol, int max_iterations) {
  double f_lo = f(lo);
  const double f_hi = f(hi);
  if ((f_lo > 0) == (f_hi > 0) && f_lo != 0 && f_hi != 0) {
    throw std::invalid_argument("Bisect needs a sign change");
  }
  RootResult r;
  for (; r.iterations < max_iterations; ++r.iterations) {
    const double mid = 0.5 * (lo + hi);
    const double f_mid = f(mid);
    if (f_mid == 0 || hi - lo < tol) {
      r.root = mid;
      r.converged = true;
      return r;
    }
    if ((f_mid > 0) == (f_lo > 0)) {
      lo = mid;
      f_lo = f_mid;
    } else {
      hi = mid;
    }
  }
  r.root = 0.5 * (lo + hi);
  return r;
}

double ImpliedVolatility(double call_price, double strike, double maturity,
                         double spot, double rate, double tol) {
  const double lower =
      std::max(spot - strike * std::exp(-rate * maturity), 0.0);
  if (!(call_price > lower && call_price < spot)) {
    throw std::domain_error("call price outside no-arbitrage bounds");
  }
  const auto price_error = [&](double vol) {
    return ch03::BlackScholesCallPrice(strike, maturity, spot, vol, rate) -
           call_price;
  };
  const auto vega = [&](double vol) {
    return ch03::BlackScholesVega(strike, maturity, spot, vol, rate);
  };
  constexpr double kLo = 1e-6, kHi = 10.0;
  // Start at the inflection point of price in vol, where Newton converges
  // monotonically for calls (Manaster-Koehler).
  const double x0 = std::clamp(
      std::sqrt(std::abs(std::log(spot / strike) + rate * maturity) * 2 /
                maturity),
      0.05, 2.0);
  const RootResult newton = NewtonRaphson(price_error, vega, x0, tol);
  if (newton.converged && newton.root > kLo && newton.root < kHi) {
    return newton.root;
  }
  return Bisect(price_error, kLo, kHi, tol).root;
}

}  // namespace cpp4q::armstrong::ch19
