#include "appendix_a_risk_neutral/binomial.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace cpp4q::armstrong::appendix_a {
namespace {

void CheckTree(double maturity, double spot, double vol, int steps) {
  if (!(maturity > 0 && spot > 0 && vol > 0) || steps < 1) {
    throw std::invalid_argument("tree needs positive inputs and steps");
  }
}

}  // namespace

double RiskNeutralProbability(double up, double down, double rate, double dt) {
  const double growth = std::exp(rate * dt);
  if (!(down < growth && growth < up)) {
    throw std::domain_error("arbitrage: need down < exp(r dt) < up");
  }
  return (growth - down) / (up - down);
}

Replication ReplicateOneStep(double spot, double up, double down, double rate,
                             double dt, double payoff_up,
                             double payoff_down) {
  if (!(up > down)) throw std::invalid_argument("need up > down");
  const double shares = (payoff_up - payoff_down) / (spot * (up - down));
  // shares * spot * up + bond * e^{r dt} = payoff_up.
  const double bond =
      (payoff_up - shares * spot * up) * std::exp(-rate * dt);
  return {shares, bond, shares * spot + bond};
}

double CrrPrice(PayoffType type, ExerciseStyle style, double strike,
                double maturity, double spot, double vol, double rate,
                int steps) {
  CheckTree(maturity, spot, vol, steps);
  const double dt = maturity / steps;
  const double u = std::exp(vol * std::sqrt(dt));
  const double d = 1 / u;
  const double q = RiskNeutralProbability(u, d, rate, dt);
  const double df = std::exp(-rate * dt);
  const bool call = type == PayoffType::kCall;
  const auto intrinsic = [&](double s) {
    return call ? std::max(s - strike, 0.0) : std::max(strike - s, 0.0);
  };
  const auto n = static_cast<std::size_t>(steps);
  // Discount folded into the branch probabilities.
  const double p_up = df * q;
  const double p_down = df * (1 - q);
  std::vector<double> v(n + 1);
  // s[j] = spot u^(2j - level) at the current level. Stepping back one
  // level multiplies each node by u, so the tree needs no pow() calls
  // after the terminal layer.
  std::vector<double> s(n + 1);
  for (std::size_t j = 0; j <= n; ++j) {
    s[j] = spot * std::pow(u, 2.0 * static_cast<double>(j) - steps);
    v[j] = intrinsic(s[j]);
  }
  const bool american = style == ExerciseStyle::kAmerican;
  for (std::size_t level = n; level-- > 0;) {
    for (std::size_t j = 0; j <= level; ++j) {
      const double cont = p_up * v[j + 1] + p_down * v[j];
      if (american) {
        s[j] *= u;
        v[j] = std::max(cont, intrinsic(s[j]));
      } else {
        v[j] = cont;
      }
    }
  }
  return v[0];
}

double CrrExpectation(const std::function<double(double)>& payoff,
                      double maturity, double spot, double vol, double rate,
                      int steps) {
  CheckTree(maturity, spot, vol, steps);
  const double dt = maturity / steps;
  const double u = std::exp(vol * std::sqrt(dt));
  const double q = RiskNeutralProbability(u, 1 / u, rate, dt);
  const double df = std::exp(-rate * dt);
  const auto n = static_cast<std::size_t>(steps);
  std::vector<double> v(n + 1);
  for (std::size_t j = 0; j <= n; ++j) {
    v[j] = payoff(spot * std::pow(u, 2.0 * static_cast<double>(j) - steps));
  }
  for (std::size_t level = n; level-- > 0;) {
    for (std::size_t j = 0; j <= level; ++j) {
      v[j] = df * (q * v[j + 1] + (1 - q) * v[j]);
    }
  }
  return v[0];
}

}  // namespace cpp4q::armstrong::appendix_a
