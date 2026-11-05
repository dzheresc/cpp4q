// Appendix A: Black-Scholes formulas.
//
// Closed forms for European calls, puts and digitals on a stock with a
// continuous dividend yield, and the call Greeks the later chapters use
// (vega drives Newton-Raphson in chapter 9). These are the yardsticks every
// Monte Carlo and tree pricer in this book is tested against.
//
// All functions take (spot, strike, r, d, vol, expiry). With zero vol or
// zero expiry they return the discounted intrinsic value of the forward.

#ifndef CPP4Q_JOSHI_APPENDIX_A_BLACK_SCHOLES_BLACK_SCHOLES_H_
#define CPP4Q_JOSHI_APPENDIX_A_BLACK_SCHOLES_BLACK_SCHOLES_H_

namespace cpp4q::joshi {

double BlackScholesCall(double spot, double strike, double r, double d,
                        double vol, double expiry);
double BlackScholesPut(double spot, double strike, double r, double d,
                       double vol, double expiry);
// Pays 1 if S(T) > strike (call) or S(T) < strike (put).
double BlackScholesDigitalCall(double spot, double strike, double r, double d,
                               double vol, double expiry);
double BlackScholesDigitalPut(double spot, double strike, double r, double d,
                              double vol, double expiry);

double BlackScholesCallDelta(double spot, double strike, double r, double d,
                             double vol, double expiry);
double BlackScholesCallGamma(double spot, double strike, double r, double d,
                             double vol, double expiry);
double BlackScholesCallVega(double spot, double strike, double r, double d,
                            double vol, double expiry);

}  // namespace cpp4q::joshi

#endif  // CPP4Q_JOSHI_APPENDIX_A_BLACK_SCHOLES_BLACK_SCHOLES_H_
