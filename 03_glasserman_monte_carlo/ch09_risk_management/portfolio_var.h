// Chapter 9: Applications in risk management.
//
// Value at risk of an option book over a short horizon (9.1-9.2): sample
// correlated lognormal moves of the underlyings, reprice everything
// (full revaluation), and read VaR and expected shortfall off the loss
// distribution. The delta-gamma-theta quadratic is the cheap proxy that
// both approximates the loss and, through its linear part, points
// importance sampling at the losses that matter: shifting the factor
// mean onto the hyperplane where the linearised loss equals the threshold
// makes a 1-in-1000 tail event common.

#ifndef CPP4Q_GLASSERMAN_CH09_RISK_MANAGEMENT_PORTFOLIO_VAR_H_
#define CPP4Q_GLASSERMAN_CH09_RISK_MANAGEMENT_PORTFOLIO_VAR_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch09 {

struct OptionPosition {
  std::size_t asset;
  ch01::OptionType type;
  double strike;
  double maturity;
  double quantity;  // negative: short
};

struct Market {
  std::vector<double> spots;
  std::vector<double> vols;
  std::vector<double> correlation;  // n x n row-major
  double rate = 0.05;
};

struct DeltaGammaTheta {
  std::vector<double> delta;  // dV/dS_j
  std::vector<double>
      gamma;     // d2V/dS_j^2 (no cross terms: single-asset options)
  double theta;  // dV/dt per year
};

class OptionPortfolio {
 public:
  OptionPortfolio(Market market, std::vector<OptionPosition> positions);

  std::size_t assets() const { return market_.spots.size(); }
  // Lower Cholesky factor of the correlation matrix, row-major.
  const std::vector<double>& correlation_factor() const { return chol_; }
  const Market& market() const { return market_; }

  // Book value at the given spots after `elapsed` years.
  double Value(std::span<const double> spots, double elapsed) const;
  double Loss(std::span<const double> spots, double horizon) const {
    return value0_ - Value(spots, horizon);
  }

  DeltaGammaTheta Sensitivities() const;
  // -(theta dt + delta . dS + 1/2 sum gamma_j dS_j^2).
  static double ApproxLoss(const DeltaGammaTheta& g, std::span<const double> ds,
                           double horizon);

  // Spot moves for horizon dt from independent normals z: correlated,
  // driftless lognormal returns.
  void Scenario(std::span<const double> z, double horizon,
                std::span<double> spots) const;

 private:
  Market market_;
  std::vector<OptionPosition> positions_;
  std::vector<double> chol_;
  double value0_;
};

// Losses from `paths` scenarios: full revaluation, or the quadratic proxy.
std::vector<double> SimulateLosses(const OptionPortfolio& book, double horizon,
                                   std::size_t paths, bool delta_gamma,
                                   std::uint64_t seed);

// Empirical (1 - alpha) quantile, e.g. alpha = 0.01 for 99% VaR.
double ValueAtRisk(std::vector<double> losses, double alpha);
// Mean loss beyond the VaR.
double ExpectedShortfall(std::vector<double> losses, double alpha);

// Eigen-decomposition of a symmetric n x n matrix (cyclic Jacobi):
// a = V diag(values) V^T, V's columns orthonormal, row-major.
struct SymmetricEigen {
  std::vector<double> values;
  std::vector<double> vectors;
};
SymmetricEigen JacobiEigen(std::vector<double> a, std::size_t n);

// The delta-gamma loss in diagonal form (9.2.2): with Z = V Y,
//   L ~ constant + sum_j (linear_j Y_j + quadratic_j Y_j^2).
struct DiagonalQuadratic {
  double constant;
  std::vector<double> linear;
  std::vector<double> quadratic;
  std::vector<double> rotation;  // V, n x n row-major: z = V y
};
DiagonalQuadratic DiagonalizeDeltaGamma(const OptionPortfolio& book,
                                        double horizon);

enum class IsMethod {
  // Shift the factor mean onto the hyperplane where the linearised loss
  // reaches the threshold. Fails when gamma dominates (e.g. a short
  // straddle: large losses lie in two opposite directions).
  kMeanShift,
  // Exponentially twist the delta-gamma quadratic (Glasserman,
  // Heidelberger and Shahabuddin): every Y_j gets a shifted mean and an
  // inflated variance, chosen so that E_theta[Q] = threshold.
  kDeltaGammaTwist,
};

// P(L > x), plain and by importance sampling; the indicator always uses
// full revaluation, so both are unbiased.
ch01::Estimate TailProbability(const OptionPortfolio& book, double horizon,
                               double threshold, std::size_t paths,
                               std::uint64_t seed);
ch01::Estimate TailProbabilityIs(const OptionPortfolio& book, double horizon,
                                 double threshold, std::size_t paths,
                                 std::uint64_t seed,
                                 IsMethod method = IsMethod::kDeltaGammaTwist);

}  // namespace cpp4q::glasserman::ch09

#endif  // CPP4Q_GLASSERMAN_CH09_RISK_MANAGEMENT_PORTFOLIO_VAR_H_
