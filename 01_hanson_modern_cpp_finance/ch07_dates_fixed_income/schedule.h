// Chapter 7: coupon schedule generation.

#ifndef CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_SCHEDULE_H_
#define CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_SCHEDULE_H_

#include <vector>

#include "ch07_dates_fixed_income/calendar.h"

namespace cpp4q::hanson::ch07 {

enum class Frequency {
  kAnnual = 1,
  kSemiAnnual = 2,
  kQuarterly = 4,
  kMonthly = 12
};

struct ScheduleSpec {
  Date effective;
  Date maturity;
  Frequency frequency = Frequency::kSemiAnnual;
  BusinessDayConvention convention = BusinessDayConvention::kModifiedFollowing;
  bool end_of_month = true;
};

// Unadjusted dates rolled backward from maturity (short front stub), then
// adjusted with the calendar. The result includes effective and maturity.
std::vector<Date> GenerateSchedule(const ScheduleSpec& spec,
                                   const Calendar& calendar);

}  // namespace cpp4q::hanson::ch07

#endif  // CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_SCHEDULE_H_
