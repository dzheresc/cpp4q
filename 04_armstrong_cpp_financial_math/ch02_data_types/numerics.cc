#include "ch02_data_types/numerics.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cpp4q::armstrong::ch02 {

std::optional<std::int64_t> CheckedAdd(std::int64_t a, std::int64_t b) {
  std::int64_t out = 0;
  if (__builtin_add_overflow(a, b, &out)) return std::nullopt;
  return out;
}

std::optional<std::int64_t> CheckedMultiply(std::int64_t a, std::int64_t b) {
  std::int64_t out = 0;
  if (__builtin_mul_overflow(a, b, &out)) return std::nullopt;
  return out;
}

std::int64_t FloorDiv(std::int64_t a, std::int64_t b) {
  if (b == 0) throw std::domain_error("division by zero");
  const std::int64_t q = a / b;
  return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

std::int64_t FloorMod(std::int64_t a, std::int64_t b) {
  return a - FloorDiv(a, b) * b;
}

double NaiveSum(std::span<const double> xs) {
  double s = 0.0;
  for (double x : xs) s += x;
  return s;
}

double CompensatedSum(std::span<const double> xs) {
  double s = 0.0;
  double c = 0.0;  // running compensation: the low-order bits lost so far
  for (double x : xs) {
    const double t = s + x;
    c += std::abs(s) >= std::abs(x) ? (s - t) + x : (x - t) + s;
    s = t;
  }
  return s + c;
}

double NaiveExpMinusOne(double x) { return std::exp(x) - 1.0; }

bool ApproxEqual(double a, double b, double rel_tol, double abs_tol) {
  return std::abs(a - b) <=
         std::max(abs_tol, rel_tol * std::max(std::abs(a), std::abs(b)));
}

}  // namespace cpp4q::armstrong::ch02
