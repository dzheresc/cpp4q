// Chapter 7: the engine, by the template-method pattern.
//
// ExoticEngine::DoSimulation is the fixed algorithm: for each path, ask the
// model for spot values at the product's look-at times, ask the product for
// its cash flows, discount them, and hand the sum to the statistics
// gatherer. The one step that varies, GetOnePath, is a pure virtual
// function, so a new model is a new subclass and nothing else changes.
// ExoticBsEngine is the Black-Scholes model with time-dependent r, d and
// vol: log-spot increments are drawn exactly, so there is no
// discretisation error.

#ifndef CPP4Q_JOSHI_CH07_EXOTICS_ENGINE_EXOTIC_ENGINE_H_
#define CPP4Q_JOSHI_CH07_EXOTICS_ENGINE_EXOTIC_ENGINE_H_

#include <cstdint>
#include <span>
#include <vector>

#include "ch04_virtual_constructor/parameters.h"
#include "ch05_strategies_statistics/statistics.h"
#include "ch05_strategies_statistics/wrapper.h"
#include "ch06_random_numbers/random.h"
#include "ch07_exotics_engine/path_dependent.h"

namespace cpp4q::joshi::ch07 {

class ExoticEngine {
 public:
  ExoticEngine(Wrapper<PathDependent> product, ch04::Parameters r);
  virtual ~ExoticEngine() = default;
  ExoticEngine(const ExoticEngine&) = delete;
  ExoticEngine& operator=(const ExoticEngine&) = delete;

  // The template method.
  void DoSimulation(ch05::StatisticsMc& gatherer, std::uint64_t num_paths);

  // Discounted value of the cash flows of one path.
  double DoOnePath(std::span<const double> spot_values) const;

  const PathDependent& product() const { return *product_; }

 protected:
  // The customisation point: spot values at product().LookAtTimes().
  virtual void GetOnePath(std::span<double> spot_values) = 0;

 private:
  Wrapper<PathDependent> product_;
  ch04::Parameters r_;
  std::vector<double> discounts_;  // at each possible cash-flow time
  mutable std::vector<CashFlow> these_cash_flows_;
};

class ExoticBsEngine final : public ExoticEngine {
 public:
  ExoticBsEngine(Wrapper<PathDependent> product, ch04::Parameters r,
                 ch04::Parameters d, ch04::Parameters vol,
                 Wrapper<ch06::RandomBase> generator, double spot);

 protected:
  void GetOnePath(std::span<double> spot_values) override;

 private:
  Wrapper<ch06::RandomBase> generator_;
  std::vector<double> drifts_;
  std::vector<double> standard_deviations_;
  double log_spot_;
  std::vector<double> variates_;
};

}  // namespace cpp4q::joshi::ch07

#endif  // CPP4Q_JOSHI_CH07_EXOTICS_ENGINE_EXOTIC_ENGINE_H_
