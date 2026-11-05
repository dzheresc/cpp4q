#include "ch07_dates_fixed_income/calendar.h"

#include <stdexcept>

namespace cpp4q::hanson::ch07 {

using std::chrono::days;
using std::chrono::sys_days;
using std::chrono::weekday;

bool IsWeekend(Date d) {
  const weekday wd{sys_days{d}};
  return wd == std::chrono::Saturday || wd == std::chrono::Sunday;
}

bool IsEndOfMonth(Date d) {
  return d.day() == (d.year() / d.month() / std::chrono::last).day();
}

Date AddMonths(Date d, int months, bool end_of_month) {
  if (!d.ok()) throw std::invalid_argument("AddMonths: invalid date");
  const bool eom = end_of_month && IsEndOfMonth(d);
  const auto ym = d.year() / d.month() + std::chrono::months{months};
  const Date last{ym / std::chrono::last};
  if (eom || d.day() > last.day()) return last;
  return ym / d.day();
}

int DaysBetween(Date a, Date b) {
  return static_cast<int>((sys_days{b} - sys_days{a}).count());
}

bool Calendar::IsBusinessDay(Date d) const {
  return !IsWeekend(d) && !holidays_.contains(sys_days{d});
}

Date Calendar::Adjust(Date d, BusinessDayConvention bdc) const {
  if (bdc == BusinessDayConvention::kUnadjusted) return d;
  sys_days day{d};
  if (bdc == BusinessDayConvention::kPreceding) {
    while (!IsBusinessDay(Date{day})) day -= days{1};
    return Date{day};
  }
  while (!IsBusinessDay(Date{day})) day += days{1};
  if (bdc == BusinessDayConvention::kModifiedFollowing &&
      Date{day}.month() != d.month()) {
    return Adjust(d, BusinessDayConvention::kPreceding);
  }
  return Date{day};
}

Date Calendar::AddBusinessDays(Date d, int n) const {
  sys_days day{d};
  const days step{n >= 0 ? 1 : -1};
  for (int remaining = n >= 0 ? n : -n; remaining > 0;) {
    day += step;
    if (IsBusinessDay(Date{day})) --remaining;
  }
  return Date{day};
}

}  // namespace cpp4q::hanson::ch07
