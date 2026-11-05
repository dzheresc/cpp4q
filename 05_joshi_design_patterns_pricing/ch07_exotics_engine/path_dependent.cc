#include "ch07_exotics_engine/path_dependent.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "appendix_a_black_scholes/black_scholes.h"
#include "appendix_b_distributions/normals.h"

namespace cpp4q::joshi::ch07 {

PathDependent::PathDependent(std::vector<double> look_at_times)
    : look_at_times_(std::move(look_at_times)) {
  if (look_at_times_.empty()) {
    throw std::invalid_argument("PathDependent: no look-at times");
  }
  double previous = 0.0;
  for (const double t : look_at_times_) {
    if (!(t > previous)) {
      throw std::invalid_argument(
          "PathDependent: look-at times must be positive and increasing");
    }
    previous = t;
  }
}

PathDependentAsian::PathDependentAsian(std::vector<double> look_at_times,
                                       double delivery_time,
                                       ch04::PayOffBridge payoff)
    : PathDependent(std::move(look_at_times)),
      delivery_time_(delivery_time),
      payoff_(std::move(payoff)) {}

std::vector<double> PathDependentAsian::PossibleCashFlowTimes() const {
  return {delivery_time_};
}

std::size_t PathDependentAsian::CashFlows(std::span<const double> spot_values,
                                          std::span<CashFlow> generated) const {
  double sum = 0.0;
  for (const double s : spot_values) sum += s;
  generated[0] = {payoff_(sum / static_cast<double>(spot_values.size())), 0};
  return 1;
}

std::unique_ptr<PathDependent> PathDependentAsian::Clone() const {
  return std::make_unique<PathDependentAsian>(*this);
}

PathDependentGeometricAsian::PathDependentGeometricAsian(
    std::vector<double> look_at_times, double delivery_time,
    ch04::PayOffBridge payoff)
    : PathDependent(std::move(look_at_times)),
      delivery_time_(delivery_time),
      payoff_(std::move(payoff)) {}

std::vector<double> PathDependentGeometricAsian::PossibleCashFlowTimes()
    const {
  return {delivery_time_};
}

std::size_t PathDependentGeometricAsian::CashFlows(
    std::span<const double> spot_values, std::span<CashFlow> generated) const {
  double sum_log = 0.0;
  for (const double s : spot_values) sum_log += std::log(s);
  generated[0] = {
      payoff_(std::exp(sum_log / static_cast<double>(spot_values.size()))), 0};
  return 1;
}

std::unique_ptr<PathDependent> PathDependentGeometricAsian::Clone() const {
  return std::make_unique<PathDependentGeometricAsian>(*this);
}

PathDependentKnockOut::PathDependentKnockOut(std::vector<double> look_at_times,
                                             double barrier,
                                             BarrierDirection direction,
                                             ch04::PayOffBridge payoff,
                                             double rebate)
    : PathDependent(std::move(look_at_times)),
      barrier_(barrier),
      direction_(direction),
      payoff_(std::move(payoff)),
      rebate_(rebate) {}

std::vector<double> PathDependentKnockOut::PossibleCashFlowTimes() const {
  return LookAtTimes();
}

std::size_t PathDependentKnockOut::CashFlows(
    std::span<const double> spot_values, std::span<CashFlow> generated) const {
  for (std::size_t i = 0; i < spot_values.size(); ++i) {
    const bool hit = direction_ == BarrierDirection::kUp
                         ? spot_values[i] >= barrier_
                         : spot_values[i] <= barrier_;
    if (hit) {
      generated[0] = {rebate_, i};
      return 1;
    }
  }
  generated[0] = {payoff_(spot_values.back()), spot_values.size() - 1};
  return 1;
}

std::unique_ptr<PathDependent> PathDependentKnockOut::Clone() const {
  return std::make_unique<PathDependentKnockOut>(*this);
}

double GeometricAsianCall(double spot, double strike, double r, double d,
                          double vol, std::span<const double> times,
                          double delivery_time) {
  const double n = static_cast<double>(times.size());
  double sum_t = 0.0;
  double sum_min = 0.0;  // sum_ij min(t_i, t_j) = sum_i (2 (n - i) - 1) t_i
  for (std::size_t i = 0; i < times.size(); ++i) {
    sum_t += times[i];
    sum_min += (2.0 * (n - static_cast<double>(i)) - 1.0) * times[i];
  }
  const double mean = std::log(spot) + (r - d - 0.5 * vol * vol) * sum_t / n;
  const double var = vol * vol * sum_min / (n * n);
  const double sd = std::sqrt(var);
  const double forward = std::exp(mean + 0.5 * var);
  const double d1 = (std::log(forward / strike) + 0.5 * var) / sd;
  return std::exp(-r * delivery_time) *
         (forward * CumulativeNormal(d1) - strike * CumulativeNormal(d1 - sd));
}

double DownAndOutCallContinuous(double spot, double strike, double barrier,
                                double r, double d, double vol,
                                double expiry) {
  if (spot <= barrier) return 0.0;
  const double vanilla = BlackScholesCall(spot, strike, r, d, vol, expiry);
  const double image =
      BlackScholesCall(barrier * barrier / spot, strike, r, d, vol, expiry);
  const double power = 1.0 - 2.0 * (r - d) / (vol * vol);
  return vanilla - std::pow(spot / barrier, power) * image;
}

}  // namespace cpp4q::joshi::ch07
