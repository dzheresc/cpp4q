// Chapter 5: Quasi-Monte Carlo.
//
// Low-discrepancy point sets: the van der Corput radical inverse, Halton
// (radical inverses in the first d primes) and Sobol (a digital sequence
// in base 2, generated in Gray-code order with one XOR per coordinate).
// Sobol uses the primitive polynomials and initial direction numbers of
// Joe and Kuo (new-joe-kuo-6.21201) for the first 16 dimensions. A random
// digital shift (XOR with a uniform 32-bit word per coordinate) keeps
// each point uniformly distributed while preserving the net structure, so
// independent shifts give confidence intervals (randomized QMC, 5.4).

#ifndef CPP4Q_GLASSERMAN_CH05_QUASI_MONTE_CARLO_LOW_DISCREPANCY_H_
#define CPP4Q_GLASSERMAN_CH05_QUASI_MONTE_CARLO_LOW_DISCREPANCY_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cpp4q::glasserman::ch05 {

// Reflects the base-b digits of n about the radix point.
double RadicalInverse(std::uint64_t n, std::uint32_t base);

std::vector<std::uint32_t> FirstPrimes(std::size_t count);

class Halton {
 public:
  explicit Halton(std::size_t dim);
  std::size_t dim() const { return bases_.size(); }
  // Point n = 1, 2, ... (index 0, the origin, is skipped).
  void Next(std::span<double> point);

 private:
  std::vector<std::uint32_t> bases_;
  std::uint64_t index_ = 0;
};

// Coefficients of a primitive polynomial over GF(2) of degree s,
// x^s + a_1 x^(s-1) + ... + a_(s-1) x + 1, with a = (a_1 ... a_(s-1)) read
// as a binary number, and the s initial direction numbers m_1..m_s.
struct SobolPolynomial {
  unsigned degree;
  unsigned a;
  std::array<std::uint32_t, 8> m;
};
std::span<const SobolPolynomial> SobolPolynomials();

class Sobol {
 public:
  static constexpr std::size_t kMaxDim = 16;
  static constexpr int kBits = 32;

  explicit Sobol(std::size_t dim);
  // With a random digital shift drawn from `shift_seed`.
  Sobol(std::size_t dim, std::uint64_t shift_seed);

  std::size_t dim() const { return dim_; }
  // Next point; the first call returns point 1 (point 0 is the origin,
  // which the inverse normal maps to -infinity when unshifted).
  void Next(std::span<double> point);
  // Positions the sequence so that the next call to Next() returns point n.
  void Seek(std::uint64_t n);

 private:
  std::size_t dim_;
  std::vector<std::array<std::uint32_t, kBits>> v_;  // direction numbers
  std::vector<std::uint32_t> x_;                     // current point
  std::vector<std::uint32_t> shift_;
  std::uint64_t index_ = 0;  // index of the point held in x_
};

// L2-star discrepancy of n points in [0,1]^d (row-major), by Warnock's
// formula; O(n^2 d).
double L2StarDiscrepancy(std::span<const double> points, std::size_t dim);

}  // namespace cpp4q::glasserman::ch05

#endif  // CPP4Q_GLASSERMAN_CH05_QUASI_MONTE_CARLO_LOW_DISCREPANCY_H_
