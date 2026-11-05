#include "ch07_dates_fixed_income/bond.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cpp4q::hanson::ch07 {

FixedRateBond::FixedRateBond(double face, double coupon_rate,
                             const ScheduleSpec& spec, DayCount day_count,
                             const Calendar& calendar)
    : face_(face),
      coupon_rate_(coupon_rate),
      frequency_(static_cast<double>(static_cast<int>(spec.frequency))),
      day_count_(day_count),
      schedule_(GenerateSchedule(spec, calendar)) {
  if (!(face > 0.0)) throw std::invalid_argument("Bond: face must be > 0");
  cash_flows_.reserve(schedule_.size());
  for (std::size_t i = 1; i < schedule_.size(); ++i) {
    const double accrual =
        YearFraction(schedule_[i - 1], schedule_[i], day_count_);
    cash_flows_.push_back({schedule_[i], face_ * coupon_rate_ * accrual});
  }
  cash_flows_.back().amount += face_;
}

double FixedRateBond::AccruedInterest(Date settle) const {
  // Find the coupon period containing settle: [schedule_[i-1], schedule_[i]).
  const auto next = std::ranges::upper_bound(schedule_, settle);
  if (next == schedule_.begin() || next == schedule_.end()) return 0.0;
  return face_ * coupon_rate_ * YearFraction(*(next - 1), settle, day_count_);
}

double FixedRateBond::DirtyPrice(Date settle, double yield) const {
  return Risk(settle, yield).dirty_price;
}

double FixedRateBond::CleanPrice(Date settle, double yield) const {
  return DirtyPrice(settle, yield) - AccruedInterest(settle);
}

std::vector<FixedRateBond::TimedFlow> FixedRateBond::FlowsAfter(
    Date settle) const {
  std::vector<TimedFlow> flows;
  flows.reserve(cash_flows_.size());
  for (const auto& cf : cash_flows_) {
    if (settle < cf.pay_date) {
      flows.push_back(
          {YearFraction(settle, cf.pay_date, day_count_), cf.amount});
    }
  }
  return flows;
}

BondRisk FixedRateBond::RiskFromFlows(const std::vector<TimedFlow>& flows,
                                      double yield) const {
  const double base = 1.0 + yield / frequency_;
  // base^(-f t) = exp(-f t log(base)): one log per call instead of a pow
  // per cash flow.
  const double minus_f_log_base = -frequency_ * std::log(base);
  double pv = 0.0;
  double pv_t = 0.0;
  double pv_t2 = 0.0;
  for (const auto& [t, amount] : flows) {
    const double v = amount * std::exp(minus_f_log_base * t);
    pv += v;
    pv_t += t * v;
    pv_t2 += t * (t + 1.0 / frequency_) * v;
  }
  BondRisk r{};
  r.dirty_price = pv;
  if (pv == 0.0) return r;
  r.macaulay_duration = pv_t / pv;
  r.modified_duration = r.macaulay_duration / base;
  r.convexity = pv_t2 / (pv * base * base);
  r.dv01 = r.modified_duration * pv * 1e-4;
  return r;
}

BondRisk FixedRateBond::Risk(Date settle, double yield) const {
  return RiskFromFlows(FlowsAfter(settle), yield);
}

double FixedRateBond::YieldFromCleanPrice(Date settle,
                                          double clean_price) const {
  const double target = clean_price + AccruedInterest(settle);
  const auto flows = FlowsAfter(settle);
  const auto f = [&](double y) {
    return RiskFromFlows(flows, y).dirty_price - target;
  };

  double y = coupon_rate_;  // par yield is a good first guess
  for (int iter = 0; iter < 50; ++iter) {
    const BondRisk r = RiskFromFlows(flows, y);
    const double diff = r.dirty_price - target;
    if (std::abs(diff) < 1e-12 * face_) return y;
    const double slope = -r.modified_duration * r.dirty_price;  // dP/dy
    if (slope == 0.0) break;
    const double next = y - diff / slope;
    if (!std::isfinite(next) || next <= -frequency_) break;
    y = next;
  }
  // Bisection fallback over a wide bracket.
  double lo = -0.99 * frequency_;
  double hi = 10.0;
  if (f(lo) * f(hi) > 0.0) {
    throw std::runtime_error("YieldFromCleanPrice: no root bracketed");
  }
  for (int iter = 0; iter < 200 && hi - lo > 1e-14; ++iter) {
    const double mid = 0.5 * (lo + hi);
    (f(lo) * f(mid) <= 0.0 ? hi : lo) = mid;
  }
  return 0.5 * (lo + hi);
}

}  // namespace cpp4q::hanson::ch07
