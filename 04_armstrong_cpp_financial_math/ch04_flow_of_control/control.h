// Chapter 4: Flow of Control.
//
// if/else, switch, for, while and do-while, each doing a small numerical
// job: series summation with a stopping rule, primality by trial
// division, bisection, the midpoint and trapezium rules, and pricing a
// call by integrating its payoff against the lognormal density (which
// must reproduce Black-Scholes).

#ifndef CPP4Q_ARMSTRONG_CH04_FLOW_OF_CONTROL_CONTROL_H_
#define CPP4Q_ARMSTRONG_CH04_FLOW_OF_CONTROL_CONTROL_H_

#include <cstdint>
#include <vector>

namespace cpp4q::armstrong::ch04 {

// n! for n <= 20 (the largest that fits in uint64); throws otherwise.
std::uint64_t Factorial(int n);

bool IsPrime(std::uint64_t n);
// All primes <= n, by the sieve of Eratosthenes.
std::vector<std::uint64_t> PrimesUpTo(std::uint64_t n);

// exp(x) by its Taylor series, summed until a term no longer changes the
// total (do-while). For x < 0 it sums at -x and inverts, which avoids
// alternating-sign cancellation.
double ExpBySeries(double x);

// Annuity: present value of `n` payments of 1 at the end of each period
// with per-period rate r. switch on the convention.
enum class AnnuityType { kImmediate, kDue };
double AnnuityPresentValue(AnnuityType type, double rate, int periods);

// Root of f in [lo, hi] by bisection; f(lo) and f(hi) must differ in
// sign. Stops when the bracket is narrower than tol.
using RealFn = double (*)(double);
double Bisection(RealFn f, double lo, double hi, double tol = 1e-12);

double IntegrateMidpoint(RealFn f, double a, double b, int steps);
double IntegrateTrapezium(RealFn f, double a, double b, int steps);

// Black-Scholes call price as discount * integral of (S_T - K)^+ times the
// risk-neutral lognormal density, by the midpoint rule in log S_T over
// [log K, mean + 10 sd].
double CallPriceByIntegration(double strike, double maturity, double spot,
                              double vol, double rate, int steps);

}  // namespace cpp4q::armstrong::ch04

#endif  // CPP4Q_ARMSTRONG_CH04_FLOW_OF_CONTROL_CONTROL_H_
