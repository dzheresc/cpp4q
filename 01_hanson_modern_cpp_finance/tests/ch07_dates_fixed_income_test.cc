#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include "ch07_dates_fixed_income/bond.h"
#include "ch07_dates_fixed_income/calendar.h"
#include "ch07_dates_fixed_income/day_count.h"
#include "ch07_dates_fixed_income/schedule.h"

namespace cpp4q::hanson::ch07 {
namespace {

using namespace std::chrono;  // NOLINT(build/namespaces) date literals

TEST(CalendarTest, WeekendsAndHolidays) {
  Calendar cal;
  cal.AddHoliday(2025y / December / 25d);
  EXPECT_FALSE(cal.IsBusinessDay(2025y / November / 8d));  // Saturday
  EXPECT_FALSE(cal.IsBusinessDay(2025y / December / 25d));
  EXPECT_TRUE(cal.IsBusinessDay(2025y / December / 24d));
}

TEST(CalendarTest, Conventions) {
  const Calendar cal;
  const Date sat = 2025y / May / 31d;  // Saturday, month end
  EXPECT_EQ(cal.Adjust(sat, BusinessDayConvention::kFollowing),
            2025y / June / 2d);
  EXPECT_EQ(cal.Adjust(sat, BusinessDayConvention::kModifiedFollowing),
            2025y / May / 30d);
  EXPECT_EQ(cal.Adjust(sat, BusinessDayConvention::kPreceding),
            2025y / May / 30d);
  EXPECT_EQ(cal.Adjust(sat, BusinessDayConvention::kUnadjusted), sat);
  EXPECT_EQ(cal.AddBusinessDays(2025y / May / 30d, 1), 2025y / June / 2d);
  EXPECT_EQ(cal.AddBusinessDays(2025y / June / 2d, -1), 2025y / May / 30d);
}

TEST(DateMathTest, AddMonthsClampsAndKeepsMonthEnd) {
  EXPECT_EQ(AddMonths(2024y / January / 31d, 1), 2024y / February / 29d);
  EXPECT_EQ(AddMonths(2023y / January / 31d, 1), 2023y / February / 28d);
  EXPECT_EQ(AddMonths(2023y / February / 28d, 1), 2023y / March / 28d);
  EXPECT_EQ(AddMonths(2023y / February / 28d, 1, true), 2023y / March / 31d);
  EXPECT_EQ(AddMonths(2023y / March / 15d, -14), 2022y / January / 15d);
  EXPECT_THROW((void)AddMonths(2023y / February / 30d, 1),
               std::invalid_argument);
}

TEST(DayCountTest, Conventions) {
  const Date a = 2024y / January / 31d;
  const Date b = 2024y / March / 31d;
  EXPECT_DOUBLE_EQ(YearFraction(a, b, DayCount::kActual360), 60.0 / 360.0);
  EXPECT_DOUBLE_EQ(YearFraction(a, b, DayCount::kActual365Fixed), 60.0 / 365.0);
  EXPECT_DOUBLE_EQ(YearFraction(a, b, DayCount::kThirty360), 60.0 / 360.0);
  EXPECT_DOUBLE_EQ(YearFraction(2024y / February / 15d, 2024y / March / 31d,
                                DayCount::kThirty360),
                   46.0 / 360.0);  // US: end day stays 31 if start < 30
  EXPECT_DOUBLE_EQ(YearFraction(2024y / February / 15d, 2024y / March / 31d,
                                DayCount::kThirtyE360),
                   45.0 / 360.0);
  // ISDA across a leap-year boundary.
  EXPECT_NEAR(YearFraction(2023y / July / 1d, 2024y / July / 1d,
                           DayCount::kActualActualIsda),
              184.0 / 365.0 + 182.0 / 366.0, 1e-15);
  EXPECT_DOUBLE_EQ(YearFraction(b, a, DayCount::kActual360), -60.0 / 360.0);
}

TEST(ScheduleTest, BackwardWithShortStubAndMonthEnd) {
  const Calendar cal;
  const auto dates =
      GenerateSchedule({.effective = 2024y / March / 15d,
                        .maturity = 2025y / June / 30d,
                        .frequency = Frequency::kSemiAnnual,
                        .convention = BusinessDayConvention::kUnadjusted},
                       cal);
  ASSERT_EQ(dates.size(), 4u);
  EXPECT_EQ(dates[0], 2024y / March / 15d);
  EXPECT_EQ(dates[1], 2024y / June / 30d);
  EXPECT_EQ(dates[2], 2024y / December / 31d);  // EOM preserved
  EXPECT_EQ(dates[3], 2025y / June / 30d);
  EXPECT_THROW((void)GenerateSchedule({.effective = 2025y / January / 1d,
                                       .maturity = 2025y / January / 1d},
                                      cal),
               std::invalid_argument);
}

class BondTest : public ::testing::Test {
 protected:
  const Calendar cal_;
  const FixedRateBond bond_{100.0,
                            0.06,
                            {.effective = 2020y / January / 15d,
                             .maturity = 2030y / January / 15d,
                             .frequency = Frequency::kSemiAnnual,
                             .convention = BusinessDayConvention::kUnadjusted},
                            DayCount::kThirty360,
                            cal_};
};

TEST_F(BondTest, ParBondOnCouponDate) {
  // On a coupon date with yield == coupon, a 30/360 bond prices at par.
  const Date settle = 2025y / January / 15d;
  EXPECT_NEAR(bond_.CleanPrice(settle, 0.06), 100.0, 1e-10);
  EXPECT_EQ(bond_.AccruedInterest(settle), 0.0);
  EXPECT_EQ(bond_.cash_flows().size(), 20u);
  EXPECT_DOUBLE_EQ(bond_.cash_flows().back().amount, 103.0);
}

TEST_F(BondTest, AccruedAndYieldRoundTrip) {
  const Date settle = 2025y / April / 15d;  // 3 of 6 months accrued
  EXPECT_NEAR(bond_.AccruedInterest(settle), 1.5, 1e-12);
  for (double y : {0.01, 0.06, 0.12, -0.005}) {
    const double clean = bond_.CleanPrice(settle, y);
    EXPECT_NEAR(bond_.YieldFromCleanPrice(settle, clean), y, 1e-10);
  }
}

TEST_F(BondTest, RiskMatchesFiniteDifferences) {
  const Date settle = 2025y / April / 15d;
  const double y = 0.05;
  const double h = 1e-5;
  const BondRisk r = bond_.Risk(settle, y);
  const double up = bond_.DirtyPrice(settle, y + h);
  const double dn = bond_.DirtyPrice(settle, y - h);
  EXPECT_NEAR(r.modified_duration, -(up - dn) / (2 * h) / r.dirty_price, 1e-6);
  EXPECT_NEAR(r.convexity,
              (up - 2 * r.dirty_price + dn) / (h * h) / r.dirty_price, 1e-3);
  EXPECT_GT(r.macaulay_duration, r.modified_duration);
  EXPECT_LT(r.macaulay_duration, 5.0);  // < time to maturity
}

TEST(CashFlowTest, ChronologicalOrdering) {
  const CashFlow a{2025y / January / 1d, 5.0};
  const CashFlow b{2025y / July / 1d, 1.0};
  EXPECT_LT(a, b);
}

}  // namespace
}  // namespace cpp4q::hanson::ch07
