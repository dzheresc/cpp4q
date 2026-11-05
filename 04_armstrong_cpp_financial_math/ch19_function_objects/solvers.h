// Chapter 19: Function Objects and Lambda Functions.
//
// The same numerical routine written against four kinds of callable: a
// function pointer, a virtual interface (chapter 10's RealFunction), a
// type-erased std::function, and a template parameter that accepts
// lambdas and functors directly. They compute the same numbers; the
// benchmarks show what each costs. Root finding with lambdas then gives
// the implied volatility, the inverse of the Black-Scholes formula.

#ifndef CPP4Q_ARMSTRONG_CH19_FUNCTION_OBJECTS_SOLVERS_H_
#define CPP4Q_ARMSTRONG_CH19_FUNCTION_OBJECTS_SOLVERS_H_

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

#include "ch10_interfaces/interfaces.h"

namespace cpp4q::armstrong::ch19 {

// A functor: a class with operator(), carrying state (the strike).
class CallPayoff {
 public:
  explicit CallPayoff(double strike) : strike_(strike) {}
  double operator()(double s) const { return std::max(s - strike_, 0.0); }

 private:
  double strike_;
};

// A lambda that captures by value: t -> exp(-rate t).
inline auto MakeDiscountFunction(double rate) {
  return [rate](double t) { return std::exp(-rate * t); };
}

// Adapts any callable to the virtual RealFunction interface.
template <typename F>
class FunctionAdapter final : public ch10::RealFunction {
 public:
  explicit FunctionAdapter(F f) : f_(std::move(f)) {}
  double Evaluate(double x) const override { return f_(x); }

 private:
  F f_;
};

// Midpoint rule, through std::function (one indirect call per point that
// the compiler cannot inline).
double IntegrateStdFunction(const std::function<double(double)>& f, double a,
                            double b, int n_points);

// Midpoint rule, templated on the callable (inlined).
template <typename F>
double IntegrateTemplate(F&& f, double a, double b, int n_points) {
  if (n_points < 1) throw std::invalid_argument("n_points must be >= 1");
  const double h = (b - a) / n_points;
  double sum = 0.0;
  for (int i = 0; i < n_points; ++i) sum += f(a + (i + 0.5) * h);
  return sum * h;
}

struct RootResult {
  double root = 0.0;
  int iterations = 0;
  bool converged = false;
};

// Bisection on a sign change in [lo, hi]; throws without one.
RootResult Bisect(const std::function<double(double)>& f, double lo,
                  double hi, double tol = 1e-12, int max_iterations = 200);

// Newton-Raphson from x0 with derivative df. Stops when |step| < tol.
template <typename F, typename DF>
RootResult NewtonRaphson(F&& f, DF&& df, double x0, double tol = 1e-12,
                         int max_iterations = 50) {
  RootResult r{x0, 0, false};
  for (; r.iterations < max_iterations; ++r.iterations) {
    const double d = df(r.root);
    if (d == 0.0 || !std::isfinite(d)) return r;
    const double step = f(r.root) / d;
    r.root -= step;
    if (std::abs(step) < tol) {
      r.converged = true;
      ++r.iterations;
      return r;
    }
  }
  return r;
}

// Black-Scholes implied volatility of a call price: Newton on vega,
// falling back to bisection on [1e-6, 10] if a step leaves the bracket.
// Throws std::domain_error if the price violates the no-arbitrage bounds
// (S - K e^{-rT})^+ < C < S.
double ImpliedVolatility(double call_price, double strike, double maturity,
                         double spot, double rate, double tol = 1e-12);

}  // namespace cpp4q::armstrong::ch19

#endif  // CPP4Q_ARMSTRONG_CH19_FUNCTION_OBJECTS_SOLVERS_H_
