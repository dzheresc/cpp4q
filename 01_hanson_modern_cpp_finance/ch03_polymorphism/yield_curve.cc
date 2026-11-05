#include "ch03_polymorphism/yield_curve.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace cpp4q::hanson::ch03 {

double YieldCurve::DiscountFactor(double t) const {
  return std::exp(-ZeroRate(t) * t);
}

double YieldCurve::ForwardRate(double t1, double t2) const {
  return (ZeroRate(t2) * t2 - ZeroRate(t1) * t1) / (t2 - t1);
}

LinearZeroCurve::LinearZeroCurve(std::vector<double> times,
                                 std::vector<double> rates)
    : times_(std::move(times)), rates_(std::move(rates)) {
  if (times_.empty() || times_.size() != rates_.size()) {
    throw std::invalid_argument("LinearZeroCurve: bad pillar sizes");
  }
  if (std::ranges::adjacent_find(times_, std::greater_equal<>()) !=
      times_.end()) {
    throw std::invalid_argument("LinearZeroCurve: times not increasing");
  }
}

double LinearZeroCurve::ZeroRate(double t) const {
  if (t <= times_.front()) return rates_.front();
  if (t >= times_.back()) return rates_.back();
  const auto hi = std::ranges::upper_bound(times_, t);
  const auto i = static_cast<std::size_t>(hi - times_.begin());
  const double w = (t - times_[i - 1]) / (times_[i] - times_[i - 1]);
  return std::lerp(rates_[i - 1], rates_[i], w);
}

}  // namespace cpp4q::hanson::ch03
