// Chapter 14: Delta Hedging.
//
// A trader sells a call at its Black-Scholes price and holds delta shares,
// rebalancing at n equally spaced dates and keeping the rest in a cash
// account at the risk-free rate. The stock follows the real-world drift,
// which hedging makes irrelevant. With the true volatility the final P&L
// has mean ~0 and a standard deviation that falls like n^(-1/2); hedging
// with the wrong volatility leaves a P&L whose sign follows
// (hedge vol - true vol) for a short option position.

#ifndef CPP4Q_ARMSTRONG_CH14_DELTA_HEDGING_HEDGING_H_
#define CPP4Q_ARMSTRONG_CH14_DELTA_HEDGING_HEDGING_H_

#include <span>
#include <vector>

#include "ch10_interfaces/interfaces.h"

namespace cpp4q::armstrong::ch14 {

using ch10::BlackScholesModel;
using ch10::EuropeanCall;
using ch10::Rng;

class HedgingSimulator {
 public:
  // `simulation_model` drives the stock (its drift and volatility);
  // `pricing_model` is what the trader believes (its volatility and rate
  // set the sale price and the deltas). Both start at the same date and
  // spot. Throws if n_steps < 1 or the option has expired.
  HedgingSimulator(const BlackScholesModel& simulation_model,
                   const BlackScholesModel& pricing_model,
                   const EuropeanCall& option, int n_steps);

  // P&L at maturity of one hedged path.
  double RunSimulation(Rng& rng) const;
  std::vector<double> RunSimulations(int n_scenarios, Rng& rng) const;

 private:
  double Simulate(ch09::PathGenerator& generator, std::span<double> path,
                  Rng& rng) const;

  BlackScholesModel simulation_model_;
  BlackScholesModel pricing_model_;
  EuropeanCall option_;
  int n_steps_;
};

}  // namespace cpp4q::armstrong::ch14

#endif  // CPP4Q_ARMSTRONG_CH14_DELTA_HEDGING_HEDGING_H_
