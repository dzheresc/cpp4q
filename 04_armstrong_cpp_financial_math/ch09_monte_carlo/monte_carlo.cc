#include "ch09_monte_carlo/monte_carlo.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cpp4q::armstrong::ch09 {

std::vector<double> GeneratePricePath(const BlackScholesModel& model,
                                      double to_date, int n_steps,
                                      Measure measure, Rng& rng) {
  if (n_steps < 1) throw std::invalid_argument("n_steps must be >= 1");
  std::vector<double> path(static_cast<std::size_t>(n_steps));
  GeneratePricePath(model, to_date, measure, rng, path);
  return path;
}

void GeneratePricePath(const BlackScholesModel& model, double to_date,
                       Measure measure, Rng& rng, std::span<double> path) {
  if (path.empty()) throw std::invalid_argument("empty path");
  if (!(to_date > model.date)) {
    throw std::invalid_argument("to_date must be after model.date");
  }
  const double dt = (to_date - model.date) / static_cast<double>(path.size());
  const double mu = measure == Measure::kRiskNeutral ? model.risk_free_rate
                                                     : model.drift;
  const double sigma = model.volatility;
  const double step_drift = (mu - 0.5 * sigma * sigma) * dt;
  const double step_sd = sigma * std::sqrt(dt);
  const std::vector<double> z =
      ch05::RandN(static_cast<int>(path.size()), rng);
  double log_s = std::log(model.stock_price);
  for (std::size_t i = 0; i < path.size(); ++i) {
    log_s += step_drift + step_sd * z[i];
    path[i] = std::exp(log_s);
  }
}

template <typename Option>
Estimate MonteCarloPricer::PriceImpl(const Option& option,
                                     const BlackScholesModel& m,
                                     Rng& rng) const {
  if (n_scenarios_ < 2) throw std::invalid_argument("need >= 2 scenarios");
  double sum = 0.0;
  double sum_sq = 0.0;
  for (int i = 0; i < n_scenarios_; ++i) {
    const std::vector<double> path =
        GeneratePricePath(m, option.maturity(), 1, Measure::kRiskNeutral, rng);
    const double payoff = option.Payoff(path.back());
    sum += payoff;
    sum_sq += payoff * payoff;
  }
  const double n = n_scenarios_;
  const double mean = sum / n;
  const double var = (sum_sq - n * mean * mean) / (n - 1);
  const double df = std::exp(-m.risk_free_rate * (option.maturity() - m.date));
  return {df * mean, df * std::sqrt(std::max(var, 0.0) / n)};
}

Estimate MonteCarloPricer::Price(const ch08::CallOption& option,
                                 const BlackScholesModel& m, Rng& rng) const {
  return PriceImpl(option, m, rng);
}

Estimate MonteCarloPricer::Price(const ch08::PutOption& option,
                                 const BlackScholesModel& m, Rng& rng) const {
  return PriceImpl(option, m, rng);
}

}  // namespace cpp4q::armstrong::ch09
