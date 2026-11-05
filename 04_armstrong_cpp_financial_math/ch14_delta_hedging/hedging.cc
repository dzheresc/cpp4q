#include "ch14_delta_hedging/hedging.h"

#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace cpp4q::armstrong::ch14 {

HedgingSimulator::HedgingSimulator(const BlackScholesModel& simulation_model,
                                   const BlackScholesModel& pricing_model,
                                   const EuropeanCall& option, int n_steps)
    : simulation_model_(simulation_model),
      pricing_model_(pricing_model),
      option_(option),
      n_steps_(n_steps) {
  if (n_steps < 1) throw std::invalid_argument("n_steps must be >= 1");
  if (!(option.maturity() > simulation_model.date)) {
    throw std::invalid_argument("option has expired");
  }
  pricing_model_.date = simulation_model_.date;
  pricing_model_.stock_price = simulation_model_.stock_price;
}

double HedgingSimulator::RunSimulation(Rng& rng) const {
  ch09::PathGenerator generator(simulation_model_, option_.maturity(),
                                n_steps_, ch09::Measure::kRealWorld);
  std::vector<double> path(static_cast<std::size_t>(n_steps_));
  return Simulate(generator, path, rng);
}

double HedgingSimulator::Simulate(ch09::PathGenerator& generator,
                                  std::span<double> path, Rng& rng) const {
  const double t0 = simulation_model_.date;
  const double dt = (option_.maturity() - t0) / n_steps_;
  const double growth = std::exp(pricing_model_.risk_free_rate * dt);
  generator.Generate(rng, path);

  BlackScholesModel belief = pricing_model_;
  double delta = option_.Delta(belief);
  double cash = option_.Price(belief) - delta * belief.stock_price;
  for (int i = 1; i < n_steps_; ++i) {
    belief.date = t0 + i * dt;
    belief.stock_price = path[static_cast<std::size_t>(i - 1)];
    cash *= growth;
    const double new_delta = option_.Delta(belief);
    cash -= (new_delta - delta) * belief.stock_price;
    delta = new_delta;
  }
  const double s_t = path.back();
  cash *= growth;
  return cash + delta * s_t - option_.PayoffAtMaturity(s_t);
}

std::vector<double> HedgingSimulator::RunSimulations(int n_scenarios,
                                                     Rng& rng) const {
  if (n_scenarios < 0) throw std::invalid_argument("n_scenarios < 0");
  ch09::PathGenerator generator(simulation_model_, option_.maturity(),
                                n_steps_, ch09::Measure::kRealWorld);
  std::vector<double> path(static_cast<std::size_t>(n_steps_));
  std::vector<double> pnl(static_cast<std::size_t>(n_scenarios));
  for (double& x : pnl) x = Simulate(generator, path, rng);
  return pnl;
}

}  // namespace cpp4q::armstrong::ch14
