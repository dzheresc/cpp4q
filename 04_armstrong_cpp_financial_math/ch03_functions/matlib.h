// Chapter 3: Functions.
//
// The book's first library of reusable functions ("matlib"): the normal
// density, distribution and its inverse, and the Black-Scholes prices
// that every later pricer is tested against. Declarations, definitions,
// pass-by-value vs pass-by-reference, overloading and default arguments
// all appear here.

#ifndef CPP4Q_ARMSTRONG_CH03_FUNCTIONS_MATLIB_H_
#define CPP4Q_ARMSTRONG_CH03_FUNCTIONS_MATLIB_H_

#include <optional>
#include <utility>

namespace cpp4q::armstrong::ch03 {

double NormPdf(double x);

// Standard normal CDF, to full double precision (via erfc).
double NormCdf(double x);

// Abramowitz & Stegun 26.2.17: a fifth-order polynomial in 1/(1+px),
// absolute error below 7.5e-8. The classic hand-rolled normcdf.
double NormCdfPolynomial(double x);

// Inverse standard normal CDF for p in (0, 1). Acklam's rational
// approximation (relative error 1.15e-9) refined by one Halley step,
// which brings it to ~1e-15. Throws std::domain_error outside (0, 1).
double NormInv(double p);

// Black-Scholes prices with continuous rate r and no dividends.
// Throw std::invalid_argument unless strike, spot, vol and maturity > 0.
double BlackScholesCallPrice(double strike, double maturity, double spot,
                             double vol, double rate);
double BlackScholesPutPrice(double strike, double maturity, double spot,
                            double vol, double rate);
// dC/dS = N(d1), dP/dS = N(d1) - 1.
double BlackScholesCallDelta(double strike, double maturity, double spot,
                             double vol, double rate);
// Common to calls and puts.
double BlackScholesGamma(double strike, double maturity, double spot,
                         double vol, double rate);
double BlackScholesVega(double strike, double maturity, double spot,
                        double vol, double rate);

// Real roots of a x^2 + b x + c, smaller first, computed without
// cancellation; nullopt if there are none (or a == 0).
std::optional<std::pair<double, double>> SolveQuadratic(double a, double b,
                                                        double c);

// Overloading: the same name for different parameter types.
inline int Square(int x) { return x * x; }
inline double Square(double x) { return x * x; }

}  // namespace cpp4q::armstrong::ch03

#endif  // CPP4Q_ARMSTRONG_CH03_FUNCTIONS_MATLIB_H_
