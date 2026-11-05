#include "ch07_dates_fixed_income/day_count.h"

#include <stdexcept>

namespace cpp4q::hanson::ch07 {
namespace {

struct Ymd {
  int y;
  int m;
  int d;
};

Ymd Split(Date date) {
  return {static_cast<int>(date.year()),
          static_cast<int>(static_cast<unsigned>(date.month())),
          static_cast<int>(static_cast<unsigned>(date.day()))};
}

double Thirty360(Ymd a, Ymd b, bool european) {
  if (european) {
    if (a.d == 31) a.d = 30;
    if (b.d == 31) b.d = 30;
  } else {
    if (a.d == 31) a.d = 30;
    if (b.d == 31 && a.d >= 30) b.d = 30;
  }
  return (360.0 * (b.y - a.y) + 30.0 * (b.m - a.m) + (b.d - a.d)) / 360.0;
}

double DaysInYear(std::chrono::year y) { return y.is_leap() ? 366.0 : 365.0; }

double ActualActualIsda(Date start, Date end) {
  if (start.year() == end.year()) {
    return DaysBetween(start, end) / DaysInYear(start.year());
  }
  // Stub in the first year + whole years + stub in the last year.
  const Date next_jan1{start.year() + std::chrono::years{1},
                       std::chrono::January, std::chrono::day{1}};
  const Date last_jan1{end.year(), std::chrono::January, std::chrono::day{1}};
  return DaysBetween(start, next_jan1) / DaysInYear(start.year()) +
         static_cast<double>(static_cast<int>(end.year()) -
                             static_cast<int>(start.year()) - 1) +
         DaysBetween(last_jan1, end) / DaysInYear(end.year());
}

}  // namespace

double YearFraction(Date start, Date end, DayCount dc) {
  if (!start.ok() || !end.ok()) {
    throw std::invalid_argument("YearFraction: invalid date");
  }
  if (end < start) return -YearFraction(end, start, dc);
  switch (dc) {
    case DayCount::kActual360:
      return DaysBetween(start, end) / 360.0;
    case DayCount::kActual365Fixed:
      return DaysBetween(start, end) / 365.0;
    case DayCount::kThirty360:
      return Thirty360(Split(start), Split(end), /*european=*/false);
    case DayCount::kThirtyE360:
      return Thirty360(Split(start), Split(end), /*european=*/true);
    case DayCount::kActualActualIsda:
      return ActualActualIsda(start, end);
  }
  throw std::invalid_argument("YearFraction: unknown day count");
}

}  // namespace cpp4q::hanson::ch07
