// Chapter 9: Solvers, templates, and implied volatilities.
//
// A root finder should work for any function without paying for a virtual
// call per evaluation, so the solvers are templates:
//
//   Bisection     any callable double(double), bracketing [low, high].
//   NewtonRaphson the book's form: the object type and pointers to its
//                 value and derivative member functions are template
//                 parameters, so both calls are resolved (and inlinable)
//                 at compile time; a second overload takes two callables.
//
// BsCall packages a call's fixed data so its price and vega are functions
// of vol alone, which is what both solvers need to find implied vols.

#ifndef CPP4Q_JOSHI_CH09_SOLVERS_IMPLIED_VOL_SOLVERS_H_
#define CPP4Q_JOSHI_CH09_SOLVERS_IMPLIED_VOL_SOLVERS_H_

#include <cmath>
#include <concepts>
#include <stdexcept>

namespace cpp4q::joshi::ch09 {

inline constexpr int kMaxIterations = 200;

// Finds x in [low, high] with |f(x) - target| <= tolerance (or the bracket
// narrower than tolerance * (1 + |x|)). Throws std::invalid_argument if
// [low, high] does not bracket the target.
template <std::invocable<double> F>
double Bisection(double target, double low, double high, double tolerance,
                 F f) {
  double f_low = f(low) - target;
  const double f_high = f(high) - target;
  if (f_low * f_high > 0.0) {
    throw std::invalid_argument("Bisection: target not bracketed");
  }
  double x = 0.5 * (low + high);
  for (int i = 0; i < 2 * kMaxIterations; ++i) {
    x = 0.5 * (low + high);
    const double y = f(x) - target;
    if (std::fabs(y) <= tolerance ||
        high - low <= tolerance * (1.0 + std::fabs(x))) {
      return x;
    }
    if ((y < 0.0) == (f_low < 0.0)) {
      low = x;
      f_low = y;
    } else {
      high = x;
    }
  }
  return x;
}

// Newton-Raphson on (the_object.*Value)(x) = target. Throws
// std::runtime_error if the derivative vanishes or it fails to converge.
template <class T, double (T::*Value)(double) const,
          double (T::*Derivative)(double) const>
double NewtonRaphson(double target, double start, double tolerance,
                     const T& the_object) {
  double x = start;
  for (int i = 0; i < kMaxIterations; ++i) {
    const double y = (the_object.*Value)(x) - target;
    if (std::fabs(y) <= tolerance) return x;
    const double dy = (the_object.*Derivative)(x);
    if (dy == 0.0 || !std::isfinite(dy)) {
      throw std::runtime_error("NewtonRaphson: zero derivative");
    }
    x -= y / dy;
  }
  throw std::runtime_error("NewtonRaphson: no convergence");
}

// The same algorithm with the function and its derivative as callables.
template <std::invocable<double> F, std::invocable<double> Df>
double NewtonRaphson(double target, double start, double tolerance, F f,
                     Df df) {
  double x = start;
  for (int i = 0; i < kMaxIterations; ++i) {
    const double y = f(x) - target;
    if (std::fabs(y) <= tolerance) return x;
    const double dy = df(x);
    if (dy == 0.0 || !std::isfinite(dy)) {
      throw std::runtime_error("NewtonRaphson: zero derivative");
    }
    x -= y / dy;
  }
  throw std::runtime_error("NewtonRaphson: no convergence");
}

// A European call as a function of volatility.
class BsCall {
 public:
  BsCall(double r, double d, double expiry, double spot, double strike)
      : r_(r), d_(d), expiry_(expiry), spot_(spot), strike_(strike) {}
  double Price(double vol) const;
  double Vega(double vol) const;
  // Manaster-Koehler: the inflection point of Price(vol), where vega peaks:
  // sqrt(2 |log(F / K)| / T).
  double InflectionVol() const;

 private:
  double r_;
  double d_;
  double expiry_;
  double spot_;
  double strike_;
};

// Implied volatility of a call price. Throws std::domain_error if the price
// is outside the no-arbitrage bounds (discounted intrinsic, spot e^{-dT}).
double ImpliedVolBisection(const BsCall& call, double price,
                           double tolerance = 1e-10);
// Newton from the Manaster-Koehler start, where vega is near its maximum,
// so iterates never overshoot into the flat tails.
double ImpliedVolNewton(const BsCall& call, double price,
                        double tolerance = 1e-10);

}  // namespace cpp4q::joshi::ch09

#endif  // CPP4Q_JOSHI_CH09_SOLVERS_IMPLIED_VOL_SOLVERS_H_
