#include <gtest/gtest.h>

#include <limits>
#include <sstream>
#include <stdexcept>

#include "ch06_unit_testing/testing.h"

namespace cpp4q::armstrong::ch06 {
namespace {

TEST(MiniFrameworkTest, AssertionsCaptureCallerLine) {
  const int line = __LINE__ + 2;
  try {
    AssertApproxEqual(1.0, 2.0, 0.5);
    FAIL() << "no throw";
  } catch (const AssertionFailure& f) {
    EXPECT_EQ(f.where().line(), static_cast<unsigned>(line));
    EXPECT_NE(f.message().find("expected 2"), std::string::npos);
  }
  EXPECT_NO_THROW(AssertApproxEqual(1.0, 1.25, 0.5));
  EXPECT_THROW(AssertApproxEqual(std::numeric_limits<double>::quiet_NaN(), 0,
                                 1e300),
               AssertionFailure);
  EXPECT_THROW(Assert(false), AssertionFailure);
  EXPECT_NO_THROW(AssertThrows([] { throw 1; }));
  EXPECT_THROW(AssertThrows([] {}), AssertionFailure);
}

TEST(MiniFrameworkTest, RegistryCountsFailuresAndErrors) {
  TestRegistry registry;
  registry.Add("passes", [] { Assert(true); });
  registry.Add("fails", [] { AssertApproxEqual(1, 2, 0.1); });
  registry.Add("errors", [] { throw std::runtime_error("boom"); });
  std::ostringstream out;
  EXPECT_EQ(registry.RunAll(out), 2);
  const std::string report = out.str();
  EXPECT_NE(report.find("[ PASS ] passes"), std::string::npos);
  EXPECT_NE(report.find("[ FAIL ] fails"), std::string::npos);
  EXPECT_NE(report.find("[ERROR ] errors: boom"), std::string::npos);
  EXPECT_NE(report.find("1/3 tests passed"), std::string::npos);
}

TEST(MiniFrameworkTest, RegistrarAddsToGlobalRegistry) {
  const std::size_t before = GlobalRegistry().size();
  const TestRegistrar r("added", [] {});
  EXPECT_EQ(GlobalRegistry().size(), before + 1);
}

}  // namespace
}  // namespace cpp4q::armstrong::ch06
