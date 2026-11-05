#include "ch07_dates_fixed_income/schedule.h"

#include <algorithm>
#include <stdexcept>

namespace cpp4q::hanson::ch07 {

std::vector<Date> GenerateSchedule(const ScheduleSpec& spec,
                                   const Calendar& calendar) {
  if (!(spec.effective < spec.maturity)) {
    throw std::invalid_argument("GenerateSchedule: effective >= maturity");
  }
  const int step = 12 / static_cast<int>(spec.frequency);
  std::vector<Date> dates{spec.maturity};
  // Roll from maturity each time (not from the previous date) so that
  // month-end clamping does not drift (Aug 31 -> Feb 28 -> Aug 28).
  for (int n = 1;; ++n) {
    const Date d = AddMonths(spec.maturity, -n * step, spec.end_of_month);
    if (!(spec.effective < d)) break;
    dates.push_back(d);
  }
  dates.push_back(spec.effective);
  std::ranges::reverse(dates);
  for (auto& d : dates) d = calendar.Adjust(d, spec.convention);
  return dates;
}

}  // namespace cpp4q::hanson::ch07
