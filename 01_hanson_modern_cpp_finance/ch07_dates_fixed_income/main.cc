// Chapter 7 demo: schedule, cash flows, price/yield and risk of a bond.

#include <chrono>
#include <format>
#include <iostream>

#include "ch07_dates_fixed_income/bond.h"

int main() {
  using namespace std::chrono;  // NOLINT(build/namespaces) for 2024y etc.
  namespace ch07 = cpp4q::hanson::ch07;

  ch07::Calendar calendar;
  calendar.AddHoliday(2026y / December / 25d);

  const ch07::ScheduleSpec spec{.effective = 2024y / February / 29d,
                                .maturity = 2029y / February / 28d,
                                .frequency = ch07::Frequency::kSemiAnnual};
  const ch07::FixedRateBond bond(100.0, 0.045, spec, ch07::DayCount::kThirty360,
                                 calendar);
  for (const auto& cf : bond.cash_flows()) {
    std::cout << std::format("{:%F}  {:9.4f}\n", sys_days{cf.pay_date},
                             cf.amount);
  }

  const ch07::Date settle = 2025y / May / 15d;
  const double ytm = 0.05;
  const auto risk = bond.Risk(settle, ytm);
  const double clean = bond.CleanPrice(settle, ytm);
  std::cout << std::format(
                   "settle {:%F}: clean {:.4f} accrued {:.4f} dirty {:.4f}\n",
                   sys_days{settle}, clean, bond.AccruedInterest(settle),
                   risk.dirty_price)
            << std::format(
                   "Macaulay {:.4f}  modified {:.4f}  convexity {:.4f}"
                   "  DV01 {:.5f}\n",
                   risk.macaulay_duration, risk.modified_duration,
                   risk.convexity, risk.dv01)
            << std::format("yield back from clean price: {:.8f}\n",
                           bond.YieldFromCleanPrice(settle, clean));
}
