// Chapter 7: a fixed-rate bullet bond priced off its yield to maturity.

#ifndef CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_BOND_H_
#define CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_BOND_H_

#include <compare>
#include <vector>

#include "ch07_dates_fixed_income/calendar.h"
#include "ch07_dates_fixed_income/day_count.h"
#include "ch07_dates_fixed_income/schedule.h"

namespace cpp4q::hanson::ch07 {

struct CashFlow {
  Date pay_date;
  double amount;
  // Dates first, so the defaulted <=> orders cash flows chronologically.
  friend auto operator<=>(const CashFlow&, const CashFlow&) = default;
};

struct BondRisk {
  double dirty_price;
  double macaulay_duration;  // years
  double modified_duration;  // years
  double convexity;          // years^2
  double dv01;               // price change for a 1bp yield move (positive)
};

class FixedRateBond {
 public:
  // Throws std::invalid_argument for non-positive face value.
  FixedRateBond(double face, double coupon_rate, const ScheduleSpec& spec,
                DayCount day_count, const Calendar& calendar);

  [[nodiscard]] const std::vector<CashFlow>& cash_flows() const {
    return cash_flows_;
  }
  [[nodiscard]] double face() const { return face_; }

  [[nodiscard]] double AccruedInterest(Date settle) const;
  // Yield compounded at the coupon frequency; time measured with the
  // bond's day count from `settle`. Only flows strictly after settle count.
  [[nodiscard]] double DirtyPrice(Date settle, double yield) const;
  [[nodiscard]] double CleanPrice(Date settle, double yield) const;
  [[nodiscard]] BondRisk Risk(Date settle, double yield) const;

  // Newton-Raphson on the dirty price with bisection fallback.
  // Throws std::runtime_error if no root is found.
  [[nodiscard]] double YieldFromCleanPrice(Date settle,
                                           double clean_price) const;

 private:
  struct TimedFlow {
    double time;  // year fraction from settle
    double amount;
  };
  // Flows after `settle` with their times. Computed once per solve: the
  // day-count arithmetic used to be redone on every Newton iteration.
  std::vector<TimedFlow> FlowsAfter(Date settle) const;
  BondRisk RiskFromFlows(const std::vector<TimedFlow>& flows,
                         double yield) const;

  double face_;
  double coupon_rate_;
  double frequency_;
  DayCount day_count_;
  std::vector<Date> schedule_;  // adjusted, includes effective date
  std::vector<CashFlow> cash_flows_;
};

}  // namespace cpp4q::hanson::ch07

#endif  // CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_BOND_H_
