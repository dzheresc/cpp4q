// Chapter 2: Basic Data Types and Operators.
//
// What the built-in types actually do, and where they bite in numerical
// finance: integer overflow (undefined behaviour for signed types, so it
// has to be checked before it happens), integer division, the spacing of
// doubles (machine epsilon, found by halving), cancellation, and summation
// error, which compensated (Kahan) summation removes.

#ifndef CPP4Q_ARMSTRONG_CH02_DATA_TYPES_NUMERICS_H_
#define CPP4Q_ARMSTRONG_CH02_DATA_TYPES_NUMERICS_H_

#include <cstdint>
#include <optional>
#include <span>

namespace cpp4q::armstrong::ch02 {

// a + b, or nullopt if the sum does not fit in int64_t.
std::optional<std::int64_t> CheckedAdd(std::int64_t a, std::int64_t b);
// a * b, or nullopt on overflow.
std::optional<std::int64_t> CheckedMultiply(std::int64_t a, std::int64_t b);

// Floor division and a modulus with the sign of the divisor (C++ `/` and
// `%` truncate toward zero, which is wrong for, e.g., day-of-week
// arithmetic on negative offsets). Throw std::domain_error if b == 0.
std::int64_t FloorDiv(std::int64_t a, std::int64_t b);
std::int64_t FloorMod(std::int64_t a, std::int64_t b);

// The smallest power of two eps with 1 + eps != 1, found by halving.
// Equals std::numeric_limits<T>::epsilon().
template <typename T>
T MachineEpsilon() {
  T eps = 1;
  // volatile stops the compiler keeping 1 + eps in an extended register.
  volatile T sum = 1 + eps / 2;
  while (sum != T{1}) {
    eps /= 2;
    sum = 1 + eps / 2;
  }
  return eps;
}

// Left-to-right sum: error grows like n * eps * sum |x|.
double NaiveSum(std::span<const double> xs);
// Kahan-Babuska (Neumaier) compensated sum: error ~ 2 eps * sum |x|,
// independent of n, and correct even when a term exceeds the running sum.
double CompensatedSum(std::span<const double> xs);

// exp(x) - 1 computed naively and via std::expm1: for small x the naive
// form loses all but a few digits to cancellation.
double NaiveExpMinusOne(double x);

// |a - b| <= max(abs_tol, rel_tol * max(|a|, |b|)).
bool ApproxEqual(double a, double b, double rel_tol = 1e-12,
                 double abs_tol = 0.0);

}  // namespace cpp4q::armstrong::ch02

#endif  // CPP4Q_ARMSTRONG_CH02_DATA_TYPES_NUMERICS_H_
