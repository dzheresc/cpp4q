// Chapter 2: Generating random numbers and random variables.
//
// Non-uniform variates by the chapter's three general methods:
// inversion (exponential, Poisson by sequential search), acceptance-
// rejection (Marsaglia-Tsang gamma), and composition/mixtures (the
// noncentral chi-square as a Poisson mixture of central ones, used for
// exact CIR simulation in chapter 3). MultivariateNormal is the
// Cholesky-factor linear transformation of 2.3.3.

#ifndef CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_DISTRIBUTIONS_H_
#define CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_DISTRIBUTIONS_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "ch02_random_numbers/normal.h"

namespace cpp4q::glasserman::ch02 {

double SampleExponential(Sampler& s, double rate);
std::uint64_t SamplePoisson(Sampler& s, double mean);
double SampleGamma(Sampler& s, double shape, double scale = 1.0);
double SampleChiSquare(Sampler& s, double dof);
double SampleNoncentralChiSquare(Sampler& s, double dof, double noncentrality);

// Lower-triangular L with L L^T = a, both n x n row-major. Throws
// std::invalid_argument if a is not symmetric positive definite.
std::vector<double> CholeskyFactor(std::span<const double> a, std::size_t n);

class MultivariateNormal {
 public:
  MultivariateNormal(std::vector<double> mean,
                     std::span<const double> covariance);
  std::size_t dim() const { return mean_.size(); }
  // Writes mean + L z into out, with z drawn from s (scratch is reused).
  void Sample(Sampler& s, std::span<double> out);
  // Same, for a caller-supplied standard normal vector z.
  void Transform(std::span<const double> z, std::span<double> out) const;

 private:
  std::vector<double> mean_;
  std::vector<double> chol_;
  std::vector<double> z_;
};

}  // namespace cpp4q::glasserman::ch02

#endif  // CPP4Q_GLASSERMAN_CH02_RANDOM_NUMBERS_DISTRIBUTIONS_H_
