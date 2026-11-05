#include "ch05_quasi_monte_carlo/low_discrepancy.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

#include "ch02_random_numbers/rng.h"

namespace cpp4q::glasserman::ch05 {
namespace {

// Dimensions 2..16 of new-joe-kuo-6.21201 (dimension 1 is van der Corput).
constexpr SobolPolynomial kPolynomials[] = {
    {1, 0, {1}},
    {2, 1, {1, 3}},
    {3, 1, {1, 3, 1}},
    {3, 2, {1, 1, 1}},
    {4, 1, {1, 1, 3, 3}},
    {4, 4, {1, 3, 5, 13}},
    {5, 2, {1, 1, 5, 5, 17}},
    {5, 4, {1, 1, 5, 5, 5}},
    {5, 7, {1, 1, 7, 11, 19}},
    {5, 11, {1, 1, 5, 1, 1}},
    {5, 13, {1, 1, 1, 3, 11}},
    {5, 14, {1, 3, 5, 5, 31}},
    {6, 1, {1, 3, 3, 9, 7, 49}},
    {6, 13, {1, 1, 1, 15, 21, 21}},
    {6, 16, {1, 3, 1, 13, 27, 49}},
};

constexpr double kTwoPow32 = 4294967296.0;

}  // namespace

double RadicalInverse(std::uint64_t n, std::uint32_t base) {
  const double inv_base = 1.0 / base;
  double scale = inv_base;
  double result = 0.0;
  while (n > 0) {
    result += static_cast<double>(n % base) * scale;
    n /= base;
    scale *= inv_base;
  }
  return result;
}

std::vector<std::uint32_t> FirstPrimes(std::size_t count) {
  std::vector<std::uint32_t> primes;
  for (std::uint32_t c = 2; primes.size() < count; ++c) {
    bool prime = true;
    for (std::uint32_t p : primes) {
      if (p * p > c) break;
      if (c % p == 0) {
        prime = false;
        break;
      }
    }
    if (prime) primes.push_back(c);
  }
  return primes;
}

Halton::Halton(std::size_t dim) : bases_(FirstPrimes(dim)) {}

void Halton::Next(std::span<double> point) {
  if (point.size() != bases_.size()) throw std::invalid_argument("bad size");
  ++index_;
  for (std::size_t j = 0; j < bases_.size(); ++j) {
    point[j] = RadicalInverse(index_, bases_[j]);
  }
}

std::span<const SobolPolynomial> SobolPolynomials() { return kPolynomials; }

Sobol::Sobol(std::size_t dim) : dim_(dim), v_(dim), x_(dim, 0), shift_(dim, 0) {
  if (dim == 0 || dim > kMaxDim) {
    throw std::invalid_argument("Sobol dimension must be in [1, 16]");
  }
  for (int i = 0; i < kBits; ++i) {
    v_[0][static_cast<std::size_t>(i)] = std::uint32_t{1} << (kBits - 1 - i);
  }
  for (std::size_t j = 1; j < dim; ++j) {
    const SobolPolynomial& p = kPolynomials[j - 1];
    const auto s = static_cast<int>(p.degree);
    auto& v = v_[j];
    for (int i = 0; i < kBits; ++i) {
      const auto ui = static_cast<std::size_t>(i);
      if (i < s) {
        v[ui] = p.m[ui] << (kBits - 1 - i);
        continue;
      }
      std::uint32_t value = v[ui - p.degree] ^ (v[ui - p.degree] >> s);
      for (int k = 1; k < s; ++k) {
        if ((p.a >> (s - 1 - k)) & 1u)
          value ^= v[ui - static_cast<std::size_t>(k)];
      }
      v[ui] = value;
    }
  }
}

Sobol::Sobol(std::size_t dim, std::uint64_t shift_seed) : Sobol(dim) {
  ch02::Mrg32k3a rng(shift_seed);
  for (auto& s : shift_) {
    s = static_cast<std::uint32_t>(rng.NextUniform() * kTwoPow32);
  }
}

void Sobol::Next(std::span<double> point) {
  if (point.size() != dim_) throw std::invalid_argument("bad size");
  // Gray code: point n+1 differs from point n in direction number c, the
  // position of the lowest zero bit of n.
  const auto c = static_cast<std::size_t>(std::countr_one(index_));
  if (c >= kBits) throw std::out_of_range("Sobol sequence exhausted");
  ++index_;
  for (std::size_t j = 0; j < dim_; ++j) {
    x_[j] ^= v_[j][c];
    point[j] = (static_cast<double>(x_[j] ^ shift_[j]) + 0.5) / kTwoPow32;
  }
}

void Sobol::Seek(std::uint64_t n) {
  if (n == 0) throw std::invalid_argument("points are numbered from 1");
  index_ = n - 1;
  const std::uint64_t gray = index_ ^ (index_ >> 1);
  for (std::size_t j = 0; j < dim_; ++j) {
    std::uint32_t x = 0;
    for (std::size_t b = 0; b < kBits; ++b) {
      if ((gray >> b) & 1u) x ^= v_[j][b];
    }
    x_[j] = x;
  }
}

double L2StarDiscrepancy(std::span<const double> points, std::size_t dim) {
  if (dim == 0 || points.size() % dim != 0) {
    throw std::invalid_argument("bad point array");
  }
  const std::size_t n = points.size() / dim;
  const auto nd = static_cast<double>(n);
  double sum1 = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    double prod = 1.0;
    for (std::size_t k = 0; k < dim; ++k) {
      const double x = points[i * dim + k];
      prod *= (1.0 - x * x);
    }
    sum1 += prod;
  }
  double sum2 = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      double prod = 1.0;
      for (std::size_t k = 0; k < dim; ++k) {
        prod *= 1.0 - std::max(points[i * dim + k], points[j * dim + k]);
      }
      sum2 += prod;
    }
  }
  const double t2 = std::pow(3.0, -static_cast<double>(dim));
  const double val = t2 -
                     std::pow(2.0, 1.0 - static_cast<double>(dim)) / nd * sum1 +
                     sum2 / (nd * nd);
  return std::sqrt(std::max(val, 0.0));
}

}  // namespace cpp4q::glasserman::ch05
