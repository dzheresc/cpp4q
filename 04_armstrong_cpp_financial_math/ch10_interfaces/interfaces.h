// Chapter 10: Interfaces.
//
// Pure abstract classes separate what a pricer needs from what a contract
// is. ContinuousTimeOption is anything with a maturity and a payoff on a
// sampled path; PathIndependentOption narrows that to payoffs of S(T)
// alone; Priceable is anything with a closed-form price. One Monte Carlo
// pricer then handles every option, and one integrator (through the
// RealFunction interface) prices any path-independent payoff by
// quadrature.

#ifndef CPP4Q_ARMSTRONG_CH10_INTERFACES_INTERFACES_H_
#define CPP4Q_ARMSTRONG_CH10_INTERFACES_INTERFACES_H_

#include <span>

#include "ch09_monte_carlo/monte_carlo.h"

namespace cpp4q::armstrong::ch10 {

using ch08::BlackScholesModel;
using ch09::Estimate;
using ch09::Rng;

class ContinuousTimeOption {
 public:
  virtual ~ContinuousTimeOption() = default;
  virtual double maturity() const = 0;
  // `prices` holds S at equally spaced dates ending at maturity.
  virtual double Payoff(std::span<const double> prices) const = 0;
  virtual bool IsPathDependent() const = 0;
};

class Priceable {
 public:
  virtual ~Priceable() = default;
  virtual double Price(const BlackScholesModel& model) const = 0;
};

class PathIndependentOption : public ContinuousTimeOption {
 public:
  double Payoff(std::span<const double> prices) const final {
    return PayoffAtMaturity(prices.back());
  }
  bool IsPathDependent() const final { return false; }
  virtual double PayoffAtMaturity(double stock_at_maturity) const = 0;
};

// Shared state of the four vanilla contracts below.
class StrikeAndMaturity {
 public:
  // Throws unless strike > 0 and maturity >= 0.
  StrikeAndMaturity(double strike, double maturity);
  double strike() const { return strike_; }

 protected:
  double TimeTo(const BlackScholesModel& m) const;  // throws if expired
  double strike_;
  double maturity_;
};

class EuropeanCall final : public PathIndependentOption,
                           public Priceable,
                           private StrikeAndMaturity {
 public:
  EuropeanCall(double strike, double maturity)
      : StrikeAndMaturity(strike, maturity) {}
  using StrikeAndMaturity::strike;
  double maturity() const override { return maturity_; }
  double PayoffAtMaturity(double s) const override;
  double Price(const BlackScholesModel& m) const override;
  double Delta(const BlackScholesModel& m) const;
};

class EuropeanPut final : public PathIndependentOption,
                          public Priceable,
                          private StrikeAndMaturity {
 public:
  EuropeanPut(double strike, double maturity)
      : StrikeAndMaturity(strike, maturity) {}
  using StrikeAndMaturity::strike;
  double maturity() const override { return maturity_; }
  double PayoffAtMaturity(double s) const override;
  double Price(const BlackScholesModel& m) const override;
};

// Pays 1 if S(T) > K.
class DigitalCall final : public PathIndependentOption,
                          public Priceable,
                          private StrikeAndMaturity {
 public:
  DigitalCall(double strike, double maturity)
      : StrikeAndMaturity(strike, maturity) {}
  using StrikeAndMaturity::strike;
  double maturity() const override { return maturity_; }
  double PayoffAtMaturity(double s) const override;
  double Price(const BlackScholesModel& m) const override;
};

// Pays 1 if S(T) < K.
class DigitalPut final : public PathIndependentOption,
                         public Priceable,
                         private StrikeAndMaturity {
 public:
  DigitalPut(double strike, double maturity)
      : StrikeAndMaturity(strike, maturity) {}
  using StrikeAndMaturity::strike;
  double maturity() const override { return maturity_; }
  double PayoffAtMaturity(double s) const override;
  double Price(const BlackScholesModel& m) const override;
};

// Monte Carlo for any ContinuousTimeOption: one exact step if the payoff
// is path independent, otherwise `n_steps` monitoring dates.
class MonteCarloPricer {
 public:
  explicit MonteCarloPricer(int n_scenarios = 10000, int n_steps = 12)
      : n_scenarios_(n_scenarios), n_steps_(n_steps) {}
  Estimate Price(const ContinuousTimeOption& option,
                 const BlackScholesModel& model, Rng& rng) const;
  int n_scenarios() const { return n_scenarios_; }
  int n_steps() const { return n_steps_; }

 private:
  int n_scenarios_;
  int n_steps_;
};

class RealFunction {
 public:
  virtual ~RealFunction() = default;
  virtual double Evaluate(double x) const = 0;
};

// Midpoint rule with n_points intervals.
double Integrate(const RealFunction& f, double a, double b, int n_points);

// Discounted risk-neutral expectation of the payoff by quadrature in
// log S(T) over mean +- 10 sd. Exact up to O(h^2), so it agrees with the
// closed forms to many digits for smooth payoffs.
double PriceByIntegration(const PathIndependentOption& option,
                          const BlackScholesModel& model, int n_points);

}  // namespace cpp4q::armstrong::ch10

#endif  // CPP4Q_ARMSTRONG_CH10_INTERFACES_INTERFACES_H_
