// Chapter 6: Unit Testing.
//
// The book builds its own minimal test framework before introducing
// anything larger, so that every function from then on ships with tests.
// This is that framework in C++20: assertions are functions (not macros)
// that capture the caller's file and line through std::source_location,
// tests register themselves in a registry, and RunAllTests reports and
// counts failures. The rest of this repository uses GoogleTest; this
// shows what such a framework does underneath.

#ifndef CPP4Q_ARMSTRONG_CH06_UNIT_TESTING_TESTING_H_
#define CPP4Q_ARMSTRONG_CH06_UNIT_TESTING_TESTING_H_

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace cpp4q::armstrong::ch06 {

// Thrown by a failed assertion; carries the caller's location.
class AssertionFailure {
 public:
  AssertionFailure(std::string message, std::source_location where);
  const std::string& message() const { return message_; }
  const std::source_location& where() const { return where_; }

 private:
  std::string message_;
  std::source_location where_;
};

void Assert(bool condition, std::string_view what = "assertion",
            std::source_location where = std::source_location::current());

// Passes if |actual - expected| <= tolerance.
void AssertApproxEqual(
    double actual, double expected, double tolerance,
    std::source_location where = std::source_location::current());

// Passes if calling f throws (anything).
void AssertThrows(
    const std::function<void()>& f,
    std::source_location where = std::source_location::current());

struct TestCase {
  std::string name;
  std::function<void()> body;
};

class TestRegistry {
 public:
  void Add(std::string name, std::function<void()> body);
  // Runs every test, printing one line per test and a summary to out.
  // Returns the number of failures. Exceptions other than
  // AssertionFailure are reported as errors and counted as failures.
  int RunAll(std::ostream& out) const;
  std::size_t size() const { return tests_.size(); }

 private:
  std::vector<TestCase> tests_;
};

// The registry used by the static TestRegistrar objects below.
TestRegistry& GlobalRegistry();

// `static TestRegistrar r("name", [] { ... });` at namespace scope adds a
// test before main runs.
struct TestRegistrar {
  TestRegistrar(std::string name, std::function<void()> body) {
    GlobalRegistry().Add(std::move(name), std::move(body));
  }
};

}  // namespace cpp4q::armstrong::ch06

#endif  // CPP4Q_ARMSTRONG_CH06_UNIT_TESTING_TESTING_H_
