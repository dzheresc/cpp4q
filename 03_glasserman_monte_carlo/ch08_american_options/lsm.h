// Chapter 8: Pricing American options.
//
// Regression-based pricing (8.6), Longstaff-Schwartz flavour: simulate
// paths forward, then step backward through the exercise dates; at each,
// regress the realised discounted cash flows of in-the-money paths on
// basis functions of the spot, and exercise where the immediate payoff
// beats the fitted continuation value.
//
// The in-sample estimate reuses the paths the rule was fitted on and is
// biased high by the foresight that brings. Applying the fitted rule to
// fresh paths gives a valid exercise policy, hence a price biased low.

#ifndef CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_LSM_H_
#define CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_LSM_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "ch01_foundations/black_scholes.h"
#include "ch01_foundations/statistics.h"

namespace cpp4q::glasserman::ch08 {

struct BermudanSpec {
  ch01::BlackScholesInputs market;
  ch01::OptionType type = ch01::OptionType::kPut;
  std::size_t exercise_dates = 10;  // at j T / n, j = 1..n
};

// Solves the k x k symmetric positive definite system a x = b in place
// (b becomes x) by Cholesky; adds a tiny ridge if a is near-singular.
void SolveNormalEquations(std::vector<double>& a, std::vector<double>& b,
                          std::size_t k);

class LsmPricer {
 public:
  // basis_size monomials 1, x, ..., x^(basis_size-1) of x = S / K.
  explicit LsmPricer(const BermudanSpec& spec, std::size_t basis_size = 4);

  // Simulates `paths` training paths, fits the exercise rule and returns
  // the in-sample estimate.
  ch01::Estimate Train(std::size_t paths, std::uint64_t seed);

  // Applies the fitted rule to fresh, independent paths.
  ch01::Estimate Price(std::size_t paths, std::uint64_t seed) const;

  // coefficients()[j] is the regression at exercise date j+1 (the last
  // date has none: exercise there whenever in the money).
  const std::vector<std::vector<double>>& coefficients() const {
    return coefficients_;
  }

 private:
  double Payoff(double s) const;
  double Continuation(std::size_t date, double s) const;
  void SimulatePaths(std::size_t paths, std::uint64_t seed,
                     std::vector<double>& spots) const;

  BermudanSpec spec_;
  std::size_t basis_size_;
  double dt_;
  double step_disc_;
  std::vector<std::vector<double>> coefficients_;
  std::vector<bool> fitted_;  // false where too few paths were in the money
};

}  // namespace cpp4q::glasserman::ch08

#endif  // CPP4Q_GLASSERMAN_CH08_AMERICAN_OPTIONS_LSM_H_
