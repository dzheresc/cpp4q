#include "ch09_monte_carlo/monte_carlo.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cpp4q::armstrong::ch09 {

PathGenerator::PathGenerator(const BlackScholesModel& model, double to_date,
                             int n_steps, Measure measure)
    : n_steps_(n_steps) {
  if (n_steps < 1) throw std::invalid_argument("n_steps must be >= 1");
  if (!(to_date > model.date)) {
    throw std::invalid_argument("to_date must be after model.date");
  }
  const double dt = (to_date - model.date) / n_steps;
  const double mu = measure == Measure::kRiskNeutral ? model.risk_free_rate
                                                     : model.drift;
  const double sigma = model.volatility;
  log_s0_ = std::log(model.stock_price);
  step_drift_ = (mu - 0.5 * sigma * sigma) * dt;
  step_sd_ = sigma * std::sqrt(dt);
}

void PathGenerator::Generate(Rng& rng, std::span<double> path) {
  if (path.size() != static_cast<std::size_t>(n_steps_)) {
    throw std::invalid_argument("path size must equal n_steps");
  }
  // Two passes: draw every normal first, then accumulate and
  // exponentiate. Fusing them into one loop was measured 25-50% slower
  // (fewer instructions, but the polar method's rejection branch then
  // sits on the log S dependency chain).
  for (double& z : path) z = normal_(rng);
  const double drift = step_drift_;
  const double sd = step_sd_;
  double log_s = log_s0_;
  for (double& s : path) {
    log_s += drift + sd * s;
    s = std::exp(log_s);
  }
}

std::vector<double> GeneratePricePath(const BlackScholesModel& model,
                                      double to_date, int n_steps,
                                      Measure measure, Rng& rng) {
  PathGenerator generator(model, to_date, n_steps, measure);
  std::vector<double> path(static_cast<std::size_t>(n_steps));
  generator.Generate(rng, path);
  return path;
}

void GeneratePricePath(const BlackScholesModel& model, double to_date,
                       Measure measure, Rng& rng, std::span<double> path) {
  if (path.empty()) throw std::invalid_argument("empty path");
  PathGenerator generator(model, to_date, static_cast<int>(path.size()),
                          measure);
  generator.Generate(rng, path);
}

template <typename Option>
Estimate MonteCarloPricer::PriceImpl(const Option& option,
                                     const BlackScholesModel& m,
                                     Rng& rng) const {
  if (n_scenarios_ < 2) throw std::invalid_argument("need >= 2 scenarios");
  PathGenerator generator(m, option.maturity(), 1, Measure::kRiskNeutral);
  double s_t = 0.0;
  double sum = 0.0;
  double sum_sq = 0.0;
  for (int i = 0; i < n_scenarios_; ++i) {
    generator.Generate(rng, std::span<double>(&s_t, 1));
    const double payoff = option.Payoff(s_t);
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
