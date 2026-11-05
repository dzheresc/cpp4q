// Chapter 7: Dates and Fixed Income Securities.
//
// std::chrono calendar types (year_month_day, sys_days, weekday) for
// business-day logic. Dates are value types and compare with <=>.

#ifndef CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_CALENDAR_H_
#define CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_CALENDAR_H_

#include <chrono>
#include <set>
#include <utility>

namespace cpp4q::hanson::ch07 {

using Date = std::chrono::year_month_day;

enum class BusinessDayConvention {
  kUnadjusted,
  kFollowing,
  kModifiedFollowing,
  kPreceding,
};

// Weekends plus an explicit holiday list.
class Calendar {
 public:
  Calendar() = default;
  explicit Calendar(std::set<std::chrono::sys_days> holidays)
      : holidays_(std::move(holidays)) {}

  void AddHoliday(Date d) { holidays_.insert(std::chrono::sys_days{d}); }
  [[nodiscard]] bool IsBusinessDay(Date d) const;
  [[nodiscard]] Date Adjust(Date d, BusinessDayConvention bdc) const;
  // Moves `n` business days (n may be negative).
  [[nodiscard]] Date AddBusinessDays(Date d, int n) const;

 private:
  std::set<std::chrono::sys_days> holidays_;
};

bool IsWeekend(Date d);

// Adds months, clamping to month end (Jan 31 + 1M = Feb 28/29). With
// end_of_month, a month-end input always maps to a month-end output.
Date AddMonths(Date d, int months, bool end_of_month = false);

bool IsEndOfMonth(Date d);

// Calendar days between two dates (b - a).
int DaysBetween(Date a, Date b);

}  // namespace cpp4q::hanson::ch07

#endif  // CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_CALENDAR_H_
