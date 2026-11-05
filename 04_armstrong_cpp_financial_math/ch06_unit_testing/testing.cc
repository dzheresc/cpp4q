#include "ch06_unit_testing/testing.h"

#include <cmath>
#include <exception>
#include <format>
#include <ostream>
#include <utility>

namespace cpp4q::armstrong::ch06 {

AssertionFailure::AssertionFailure(std::string message,
                                   std::source_location where)
    : message_(std::move(message)), where_(where) {}

void Assert(bool condition, std::string_view what,
            std::source_location where) {
  if (!condition) throw AssertionFailure(std::string(what) + " failed", where);
}

void AssertApproxEqual(double actual, double expected, double tolerance,
                       std::source_location where) {
  // Written so that NaN fails.
  if (!(std::abs(actual - expected) <= tolerance)) {
    throw AssertionFailure(
        std::format("expected {} but got {} (tolerance {})", expected, actual,
                    tolerance),
        where);
  }
}

void AssertThrows(const std::function<void()>& f, std::source_location where) {
  try {
    f();
  } catch (...) {
    return;
  }
  throw AssertionFailure("expected an exception", where);
}

void TestRegistry::Add(std::string name, std::function<void()> body) {
  tests_.push_back({std::move(name), std::move(body)});
}

int TestRegistry::RunAll(std::ostream& out) const {
  int failures = 0;
  for (const TestCase& t : tests_) {
    try {
      t.body();
      out << "[ PASS ] " << t.name << '\n';
    } catch (const AssertionFailure& f) {
      ++failures;
      out << "[ FAIL ] " << t.name << ": " << f.message() << " at "
          << f.where().file_name() << ':' << f.where().line() << '\n';
    } catch (const std::exception& e) {
      ++failures;
      out << "[ERROR ] " << t.name << ": " << e.what() << '\n';
    }
  }
  out << tests_.size() - static_cast<std::size_t>(failures) << '/'
      << tests_.size() << " tests passed\n";
  return failures;
}

TestRegistry& GlobalRegistry() {
  static TestRegistry registry;  // constructed on first use
  return registry;
}

}  // namespace cpp4q::armstrong::ch06
