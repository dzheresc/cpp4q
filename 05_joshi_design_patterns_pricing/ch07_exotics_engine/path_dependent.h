// Chapter 7: An exotics engine and the template pattern (products).
//
// A path-dependent product is described by what it needs and what it pays:
// the times at which it looks at the spot, the times at which it may pay,
// and a function from one path of spot values to the cash flows it
// generates. It knows nothing about models or discounting, so the same
// product runs under any engine.

#ifndef CPP4Q_JOSHI_CH07_EXOTICS_ENGINE_PATH_DEPENDENT_H_
#define CPP4Q_JOSHI_CH07_EXOTICS_ENGINE_PATH_DEPENDENT_H_

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "ch04_virtual_constructor/payoff_bridge.h"

namespace cpp4q::joshi::ch07 {

struct CashFlow {
  double amount = 0.0;
  // Index into the product's PossibleCashFlowTimes().
  std::size_t time_index = 0;
};

class PathDependent {
 public:
  // Throws unless `look_at_times` is non-empty, positive and increasing.
  explicit PathDependent(std::vector<double> look_at_times);
  virtual ~PathDependent() = default;

  const std::vector<double>& LookAtTimes() const { return look_at_times_; }

  virtual std::size_t MaxNumberOfCashFlows() const = 0;
  virtual std::vector<double> PossibleCashFlowTimes() const = 0;
  // `spot_values[i]` is S(LookAtTimes()[i]); `generated` has room for
  // MaxNumberOfCashFlows() entries. Returns how many were written.
  virtual std::size_t CashFlows(std::span<const double> spot_values,
                                std::span<CashFlow> generated) const = 0;
  virtual std::unique_ptr<PathDependent> Clone() const = 0;

 protected:
  PathDependent(const PathDependent&) = default;
  PathDependent& operator=(const PathDependent&) = default;

 private:
  std::vector<double> look_at_times_;
};

// Pays payoff(arithmetic mean of the fixings) at `delivery_time`.
class PathDependentAsian final : public PathDependent {
 public:
  PathDependentAsian(std::vector<double> look_at_times, double delivery_time,
                     ch04::PayOffBridge payoff);
  std::size_t MaxNumberOfCashFlows() const override { return 1; }
  std::vector<double> PossibleCashFlowTimes() const override;
  std::size_t CashFlows(std::span<const double> spot_values,
                        std::span<CashFlow> generated) const override;
  std::unique_ptr<PathDependent> Clone() const override;

 private:
  double delivery_time_;
  ch04::PayOffBridge payoff_;
};

// Pays payoff(geometric mean of the fixings) at `delivery_time`; it has a
// closed form (GeometricAsianCall below) and is the control variate for
// the arithmetic Asian.
class PathDependentGeometricAsian final : public PathDependent {
 public:
  PathDependentGeometricAsian(std::vector<double> look_at_times,
                              double delivery_time,
                              ch04::PayOffBridge payoff);
  std::size_t MaxNumberOfCashFlows() const override { return 1; }
  std::vector<double> PossibleCashFlowTimes() const override;
  std::size_t CashFlows(std::span<const double> spot_values,
                        std::span<CashFlow> generated) const override;
  std::unique_ptr<PathDependent> Clone() const override;

 private:
  double delivery_time_;
  ch04::PayOffBridge payoff_;
};

enum class BarrierDirection { kUp, kDown };

// Discretely monitored knock-out: if the spot is at or beyond `barrier` at a
// look-at time, pays `rebate` at that time and dies; otherwise pays
// payoff(S(T)) at the last look-at time. Cash flows can therefore occur at
// any of the look-at times.
class PathDependentKnockOut final : public PathDependent {
 public:
  PathDependentKnockOut(std::vector<double> look_at_times, double barrier,
                        BarrierDirection direction, ch04::PayOffBridge payoff,
                        double rebate = 0.0);
  std::size_t MaxNumberOfCashFlows() const override { return 1; }
  std::vector<double> PossibleCashFlowTimes() const override;
  std::size_t CashFlows(std::span<const double> spot_values,
                        std::span<CashFlow> generated) const override;
  std::unique_ptr<PathDependent> Clone() const override;

 private:
  double barrier_;
  BarrierDirection direction_;
  ch04::PayOffBridge payoff_;
  double rebate_;
};

// Closed form of the discretely sampled geometric-average call (constant
// r, d, vol): log G is Gaussian with mean log S + sum_i (r - d - vol^2/2)
// t_i / n and variance vol^2 / n^2 sum_ij min(t_i, t_j).
double GeometricAsianCall(double spot, double strike, double r, double d,
                          double vol, std::span<const double> times,
                          double delivery_time);

// Continuously monitored down-and-out call with barrier <= strike and no
// rebate, by the reflection principle:
// C - (S / H)^(1 - 2 (r - d) / vol^2) C(H^2 / S).
double DownAndOutCallContinuous(double spot, double strike, double barrier,
                                double r, double d, double vol,
                                double expiry);

}  // namespace cpp4q::joshi::ch07

#endif  // CPP4Q_JOSHI_CH07_EXOTICS_ENGINE_PATH_DEPENDENT_H_
