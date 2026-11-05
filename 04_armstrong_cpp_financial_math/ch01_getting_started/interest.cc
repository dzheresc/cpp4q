#include "ch01_getting_started/interest.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cpp4q::armstrong::ch01 {
namespace {

void CheckPeriods(int periods_per_year) {
  if (periods_per_year < 1) {
    throw std::invalid_argument("periods_per_year must be at least 1");
  }
}

}  // namespace

double SimpleInterest(double principal, double rate, double years) {
  return principal * (1.0 + rate * years);
}

double CompoundInterest(double principal, double rate, double years,
                        int periods_per_year) {
  CheckPeriods(periods_per_year);
  const double m = periods_per_year;
  // pow(1 + r/m, m t) loses digits for large m; exp(m t log1p(r/m)) does not.
  return principal * std::exp(m * years * std::log1p(rate / m));
}

double ContinuousCompounding(double principal, double rate, double years) {
  return principal * std::exp(rate * years);
}

double ContinuousEquivalentRate(double rate, int periods_per_year) {
  CheckPeriods(periods_per_year);
  const double m = periods_per_year;
  return m * std::log1p(rate / m);
}

double DoublingTime(double rate) {
  if (!(rate > 0.0)) throw std::invalid_argument("rate must be positive");
  return std::numbers::ln2 / rate;
}

}  // namespace cpp4q::armstrong::ch01
