#include "ch04_flow_of_control/control.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

#include "ch03_functions/matlib.h"

namespace cpp4q::armstrong::ch04 {

std::uint64_t Factorial(int n) {
  if (n < 0 || n > 20) throw std::out_of_range("Factorial needs 0 <= n <= 20");
  std::uint64_t result = 1;
  for (int i = 2; i <= n; ++i) result *= static_cast<std::uint64_t>(i);
  return result;
}

bool IsPrime(std::uint64_t n) {
  if (n < 2) return false;
  if (n < 4) return true;
  if (n % 2 == 0 || n % 3 == 0) return false;
  for (std::uint64_t d = 5; d <= n / d; d += 6) {
    if (n % d == 0 || n % (d + 2) == 0) return false;
  }
  return true;
}

std::vector<std::uint64_t> PrimesUpTo(std::uint64_t n) {
  std::vector<std::uint64_t> primes;
  if (n < 2) return primes;
  std::vector<bool> composite(n + 1, false);
  for (std::uint64_t i = 2; i <= n; ++i) {
    if (composite[i]) continue;
    primes.push_back(i);
    for (std::uint64_t j = i * i; j <= n; j += i) composite[j] = true;
  }
  return primes;
}

double ExpBySeries(double x) {
  if (x < 0) return 1.0 / ExpBySeries(-x);
  double sum = 1.0;
  double term = 1.0;
  double previous;
  int k = 0;
  do {
    previous = sum;
    ++k;
    term *= x / k;
    sum += term;
  } while (sum != previous);
  return sum;
}

double AnnuityPresentValue(AnnuityType type, double rate, int periods) {
  if (periods < 0) throw std::invalid_argument("periods must be >= 0");
  double pv = 0.0;
  double discount = 1.0 / (1.0 + rate);
  double df = 1.0;
  switch (type) {
    case AnnuityType::kImmediate:
      for (int i = 1; i <= periods; ++i) {
        df *= discount;
        pv += df;
      }
      break;
    case AnnuityType::kDue:
      for (int i = 0; i < periods; ++i) {
        pv += df;
        df *= discount;
      }
      break;
  }
  return pv;
}

double Bisection(RealFn f, double lo, double hi, double tol) {
  double f_lo = f(lo);
  const double f_hi = f(hi);
  if (f_lo == 0) return lo;
  if (f_hi == 0) return hi;
  if ((f_lo > 0) == (f_hi > 0)) {
    throw std::invalid_argument("Bisection needs a sign change");
  }
  while (hi - lo > tol) {
    const double mid = 0.5 * (lo + hi);
    if (mid <= lo || mid >= hi) break;  // no representable midpoint left
    const double f_mid = f(mid);
    if (f_mid == 0) return mid;
    if ((f_mid > 0) == (f_lo > 0)) {
      lo = mid;
      f_lo = f_mid;
    } else {
      hi = mid;
    }
  }
  return 0.5 * (lo + hi);
}

double IntegrateMidpoint(RealFn f, double a, double b, int steps) {
  if (steps < 1) throw std::invalid_argument("steps must be >= 1");
  const double h = (b - a) / steps;
  double sum = 0.0;
  for (int i = 0; i < steps; ++i) sum += f(a + (i + 0.5) * h);
  return sum * h;
}

double IntegrateTrapezium(RealFn f, double a, double b, int steps) {
  if (steps < 1) throw std::invalid_argument("steps must be >= 1");
  const double h = (b - a) / steps;
  double sum = 0.5 * (f(a) + f(b));
  for (int i = 1; i < steps; ++i) sum += f(a + i * h);
  return sum * h;
}

double CallPriceByIntegration(double strike, double maturity, double spot,
                              double vol, double rate, int steps) {
  if (steps < 1) throw std::invalid_argument("steps must be >= 1");
  // log S_T ~ N(m, s^2).
  const double s = vol * std::sqrt(maturity);
  const double m = std::log(spot) + (rate - 0.5 * vol * vol) * maturity;
  const double lo = std::log(strike);
  const double hi = std::max(lo, m) + 10 * s;
  const double h = (hi - lo) / steps;
  double sum = 0.0;
  for (int i = 0; i < steps; ++i) {
    const double y = lo + (i + 0.5) * h;  // y = log S_T
    sum += (std::exp(y) - strike) * ch03::NormPdf((y - m) / s);
  }
  return std::exp(-rate * maturity) * sum * h / s;
}

}  // namespace cpp4q::armstrong::ch04
