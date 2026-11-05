// Chapter 7: day-count conventions.

#ifndef CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_DAY_COUNT_H_
#define CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_DAY_COUNT_H_

#include "ch07_dates_fixed_income/calendar.h"

namespace cpp4q::hanson::ch07 {

enum class DayCount {
  kActual360,
  kActual365Fixed,
  kThirty360,   // 30/360 US bond basis
  kThirtyE360,  // 30E/360 (Eurobond)
  kActualActualIsda,
};

// Accrual fraction between `start` and `end` (start <= end).
double YearFraction(Date start, Date end, DayCount dc);

}  // namespace cpp4q::hanson::ch07

#endif  // CPP4Q_HANSON_CH07_DATES_FIXED_INCOME_DAY_COUNT_H_
