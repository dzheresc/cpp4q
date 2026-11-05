// Chapter 1: Foundations.
//
// Closed-form prices that the Monte Carlo estimators in this book are
// checked against: Black-Scholes calls and puts with their Greeks, a
// cash-or-nothing digital, and the discretely monitored geometric-average
// Asian call (the control variate of chapter 4).

#ifndef CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_BLACK_SCHOLES_H_
#define CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_BLACK_SCHOLES_H_

namespace cpp4q::glasserman::ch01 {

inline constexpr double kSqrt2 = 1.41421356237309504880;
inline constexpr double kInvSqrt2Pi = 0.39894228040143267794;

double NormalPdf(double x);
double NormalCdf(double x);

enum class OptionType { kCall, kPut };

// Geometric Brownian motion under the risk-neutral measure with
// continuous dividend yield: dS = (rate - dividend) S dt + vol S dW.
struct BlackScholesInputs {
  double spot = 100.0;
  double strike = 100.0;
  double rate = 0.05;
  double vol = 0.2;
  double maturity = 1.0;
  double dividend = 0.0;
};

double BlackScholesPrice(OptionType type, const BlackScholesInputs& in);
double BlackScholesDelta(OptionType type, const BlackScholesInputs& in);
double BlackScholesGamma(const BlackScholesInputs& in);
double BlackScholesVega(const BlackScholesInputs& in);

// Pays 1 at maturity if S(T) > K.
double DigitalCallPrice(const BlackScholesInputs& in);
// d(DigitalCallPrice)/d(spot).
double DigitalCallDelta(const BlackScholesInputs& in);

// Call on the geometric average of S(t_1), ..., S(t_n), t_i = i T / n.
// log G is normal, so this has a Black-Scholes-type formula.
double GeometricAsianCallPrice(const BlackScholesInputs& in, int fixings);

}  // namespace cpp4q::glasserman::ch01

#endif  // CPP4Q_GLASSERMAN_CH01_FOUNDATIONS_BLACK_SCHOLES_H_
