// Chapter 9: Monte Carlo Pricing in C++.
//
// Simulating the Black-Scholes model: GeneratePricePath samples the stock
// at equally spaced dates (under the real-world drift or the risk-neutral
// rate), and MonteCarloPricer averages discounted payoffs, reporting the
// standard error alongside the price so a reader can tell whether a
// difference from Black-Scholes is real.

#ifndef CPP4Q_ARMSTRONG_CH09_MONTE_CARLO_MONTE_CARLO_H_
#define CPP4Q_ARMSTRONG_CH09_MONTE_CARLO_MONTE_CARLO_H_

#include <span>
#include <vector>

#include "ch05_multiple_files/vectors.h"
#include "ch08_user_defined_types/options.h"

namespace cpp4q::armstrong::ch09 {

using ch05::Rng;
using ch08::BlackScholesModel;

enum class Measure { kRealWorld, kRiskNeutral };

// S(t_1), ..., S(t_n) with t_i = date + i (to_date - date) / n: exact
// lognormal steps, so there is no discretisation error at the dates.
std::vector<double> GeneratePricePath(const BlackScholesModel& model,
                                      double to_date, int n_steps,
                                      Measure measure, Rng& rng);

// As above, writing into `path` (whose size is the number of steps) so
// that a loop over scenarios can reuse one buffer.
void GeneratePricePath(const BlackScholesModel& model, double to_date,
                       Measure measure, Rng& rng, std::span<double> path);

struct Estimate {
  double price = 0.0;
  double std_error = 0.0;
  // True if x lies within `z` standard errors of the estimate.
  bool Contains(double x, double z = 4.0) const {
    return price - z * std_error <= x && x <= price + z * std_error;
  }
};

// Plain Monte Carlo for a European call or put: one exact step to
// maturity per scenario.
class MonteCarloPricer {
 public:
  explicit MonteCarloPricer(int n_scenarios = 10000)
      : n_scenarios_(n_scenarios) {}

  int n_scenarios() const { return n_scenarios_; }

  Estimate Price(const ch08::CallOption& option, const BlackScholesModel& m,
                 Rng& rng) const;
  Estimate Price(const ch08::PutOption& option, const BlackScholesModel& m,
                 Rng& rng) const;

 private:
  template <typename Option>
  Estimate PriceImpl(const Option& option, const BlackScholesModel& m,
                     Rng& rng) const;
  int n_scenarios_;
};

}  // namespace cpp4q::armstrong::ch09

#endif  // CPP4Q_ARMSTRONG_CH09_MONTE_CARLO_MONTE_CARLO_H_
