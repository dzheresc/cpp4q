// Chapter 9: Applications in risk management.
//
// Credit portfolio losses in the one-factor Gaussian copula (9.4):
// obligor k defaults when a_k Y + sqrt(1 - a_k^2) e_k < Phi^-1(p_k), with a
// common factor Y and idiosyncratic e_k. Large losses need both a bad Y
// and many defaults given Y, so Glasserman and Li's importance sampler
// does two things: shift the mean of Y (to the mode of the optimal
// zero-variance density, approximately), then exponentially twist the
// conditionally independent default indicators so that the conditional
// expected loss equals the threshold.

#ifndef CPP4Q_GLASSERMAN_CH09_RISK_MANAGEMENT_CREDIT_H_
#define CPP4Q_GLASSERMAN_CH09_RISK_MANAGEMENT_CREDIT_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch09 {

struct Obligor {
  double default_prob;
  double exposure;  // loss given default, in currency
  double loading;   // a_k in [0, 1)
};

class GaussianCopulaPortfolio {
 public:
  explicit GaussianCopulaPortfolio(std::vector<Obligor> obligors);

  std::size_t size() const { return obligors_.size(); }
  double ExpectedLoss() const;
  double ConditionalDefaultProb(std::size_t k, double y) const;

  // P(L > x).
  ch01::Estimate TailProbability(double x, std::size_t paths,
                                 std::uint64_t seed) const;
  // Glasserman-Li two-step importance sampling with factor mean `shift`.
  ch01::Estimate TailProbabilityIs(double x, double shift, std::size_t paths,
                                   std::uint64_t seed) const;
  // argmax over y of F(y) - y^2 / 2, F(y) = min_theta (psi(theta, y) - theta
  // x).
  double FindFactorShift(double x) const;

  // Twisting parameter for conditional probabilities p: theta >= 0 such
  // that the twisted expected loss is x (0 if it already exceeds x).
  double SolveTheta(const std::vector<double>& p, double x) const;

 private:
  double Psi(const std::vector<double>& p, double theta) const;

  std::vector<Obligor> obligors_;
  std::vector<double> thresholds_;  // Phi^-1(p_k)
  std::vector<double> idio_sd_;     // sqrt(1 - a_k^2)
};

}  // namespace cpp4q::glasserman::ch09

#endif  // CPP4Q_GLASSERMAN_CH09_RISK_MANAGEMENT_CREDIT_H_
