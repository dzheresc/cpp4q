#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <limits>
#include <source_location>
#include <sstream>
#include <string>

#include "ch15_debugging_tools/debug.h"

namespace cpp4q::armstrong::ch15 {
namespace {

TEST(RequireTest, ReportsCallerLocation) {
  const int line = __LINE__ + 2;
  try {
    Require(false, "spot must be positive");
    FAIL();
  } catch (const PreconditionError& e) {
    const std::string what = e.what();
    EXPECT_NE(what.find("ch15_debugging_tools_test.cc:" +
                        std::to_string(line)),
              std::string::npos)
        << what;
    EXPECT_NE(what.find("spot must be positive"), std::string::npos);
  }
  EXPECT_NO_THROW(Require(true, "fine"));
}

TEST(RequireTest, Finite) {
  EXPECT_EQ(RequireFinite(1.5, "x"), 1.5);
  EXPECT_THROW(RequireFinite(std::nan(""), "price"), PreconditionError);
  EXPECT_THROW(RequireFinite(std::numeric_limits<double>::infinity(), "p"),
               PreconditionError);
}

TEST(LoggerTest, ThresholdFilters) {
  std::ostringstream out;
  Logger log(out, LogLevel::kWarning);
  log.Log(LogLevel::kInfo, std::source_location::current(), "hidden {}", 1);
  log.Log(LogLevel::kError, std::source_location::current(), "shown {}", 42);
  EXPECT_EQ(out.str().find("hidden"), std::string::npos);
  EXPECT_NE(out.str().find("[ERROR] ch15_debugging_tools_test.cc:"),
            std::string::npos);
  EXPECT_NE(out.str().find("shown 42"), std::string::npos);
  log.set_threshold(LogLevel::kDebug);
  EXPECT_TRUE(log.Enabled(LogLevel::kDebug));
}

TEST(TimerTest, ReportsOnDestruction) {
  bool called = false;
  ScopedTimer::Clock::duration seen{-1};
  {
    const ScopedTimer t([&](ScopedTimer::Clock::duration d) {
      called = true;
      seen = d;
    });
    EXPECT_FALSE(called);
  }
  EXPECT_TRUE(called);
  EXPECT_GE(seen.count(), 0);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch15
